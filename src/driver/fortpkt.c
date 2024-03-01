/* Fort Firewall Packets Shaping & Re-injection */

#include "fortpkt.h"

#include "fortdbg.h"
#include "fortdev.h"
#include "forttrace.h"
#include "fortutl.h"

#define FORT_PACKET_POOL_TAG 'KwfF'

#define FORT_PACKET_FLUSH_ALL 0xFFFFFFFF

#define FORT_QUEUE_INITIAL_TOKEN_COUNT 1500

#define HTONL(l) _byteswap_ulong(l)

typedef void FORT_SHAPER_PACKET_FOREACH_FUNC(PFORT_SHAPER, PFORT_FLOW_PACKET);

static UCHAR fort_shaper_flags_set(PFORT_SHAPER shaper, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&shaper->flags, flags) : InterlockedAnd8(&shaper->flags, ~flags);
}

static UCHAR fort_shaper_flags(PFORT_SHAPER shaper)
{
    return fort_shaper_flags_set(shaper, 0, TRUE);
}

static LONG fort_shaper_io_bits_exchange(volatile LONG *io_bits, LONG v)
{
    return InterlockedExchange(io_bits, v);
}

static LONG fort_shaper_io_bits_set(volatile LONG *io_bits, LONG v, BOOL on)
{
    return on ? InterlockedOr(io_bits, v) : InterlockedAnd(io_bits, ~v);
}

static LONG fort_shaper_io_bits(volatile LONG *io_bits)
{
    return fort_shaper_io_bits_set(io_bits, 0, TRUE);
}

inline static HANDLE fort_packet_injection_id(BOOL isIPv6, BOOL inbound)
{
    PFORT_PENDING pending = &fort_device()->pending;

    return isIPv6
            ? (inbound ? pending->injection_transport6_in_id : pending->injection_transport6_out_id)
            : (inbound ? pending->injection_transport4_in_id
                       : pending->injection_transport4_out_id);
}

static BOOL fort_packet_injected_by_self(PCFORT_CALLOUT_ARG ca)
{
    if (ca->netBufList == NULL)
        return FALSE;

    const HANDLE injection_id = fort_packet_injection_id(ca->isIPv6, ca->inbound);

    const FWPS_PACKET_INJECTION_STATE state =
            FwpsQueryPacketInjectionState0(injection_id, ca->netBufList, NULL);

    return (state == FWPS_PACKET_INJECTED_BY_SELF
            || state == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF);
}

static FWPS_PACKET_LIST_INBOUND_IPSEC_INFORMATION0 fort_packet_get_ipsec_inbound_info(
        PCFORT_CALLOUT_ARG ca)
{
    if (!ca->inbound || ca->netBufList == NULL) {
        const FWPS_PACKET_LIST_INBOUND_IPSEC_INFORMATION0 info = { 0 };
        return info;
    }

    FWPS_PACKET_LIST_INFORMATION0 packet_info;
    RtlZeroMemory(&packet_info, sizeof(FWPS_PACKET_LIST_INFORMATION0));

    FwpsGetPacketListSecurityInformation0(ca->netBufList,
            FWPS_PACKET_LIST_INFORMATION_QUERY_IPSEC | FWPS_PACKET_LIST_INFORMATION_QUERY_INBOUND,
            &packet_info);

    return packet_info.ipsecInformation.inbound;
}

inline static BOOL fort_packet_is_ipsec_protected(PCFORT_CALLOUT_ARG ca)
{
    const FWPS_PACKET_LIST_INBOUND_IPSEC_INFORMATION0 info = fort_packet_get_ipsec_inbound_info(ca);
    return info.isSecure;
}

inline static BOOL fort_packet_is_ipsec_tunneled(PCFORT_CALLOUT_ARG ca)
{
    const FWPS_PACKET_LIST_INBOUND_IPSEC_INFORMATION0 info = fort_packet_get_ipsec_inbound_info(ca);
    return info.isTunnelMode && !info.isDeTunneled;
}

inline static ULONG fort_packet_data_length(PCFORT_CALLOUT_ARG ca)
{
    PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(ca->netBufList);
    const ULONG data_length = NET_BUFFER_DATA_LENGTH(netBuf);

    const ULONG header_size = ca->inbound
            ? 0
            : ca->inMetaValues->transportHeaderSize + ca->inMetaValues->ipHeaderSize;

    return header_size + data_length;
}

inline static PFORT_FLOW_PACKET fort_shaper_packet_new(void)
{
    return fort_mem_alloc(sizeof(FORT_FLOW_PACKET), FORT_PACKET_POOL_TAG);
}

inline static void fort_shaper_packet_del(PFORT_FLOW_PACKET pkt)
{
    fort_mem_free(pkt, FORT_PACKET_POOL_TAG);
}

static void fort_packet_free_cloned(PNET_BUFFER_LIST clonedNetBufList)
{
    if (clonedNetBufList == NULL)
        return;

    const NTSTATUS status = clonedNetBufList->Status;

    if (!NT_SUCCESS(status) && status != STATUS_NOT_FOUND) {
        LOG("Shaper: Packet injection error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_INJECTION_ERROR, status, 0, 0);
    }

    FwpsFreeCloneNetBufferList0(clonedNetBufList, 0);
}

inline static void fort_packet_free_out(PFORT_PACKET_OUT pkt)
{
    if (pkt->controlData != NULL) {
        fort_mem_free(pkt->controlData, FORT_PACKET_POOL_TAG);
    }
}

static void fort_packet_free(PFORT_PACKET_IO pkt)
{
    if ((pkt->flags & FORT_PACKET_INBOUND) == 0) {
        fort_packet_free_out(&pkt->out);
    }

    fort_packet_free_cloned(pkt->netBufList);
}

static void fort_shaper_packet_free(PFORT_FLOW_PACKET pkt)
{
    fort_packet_free(&pkt->io);

    fort_shaper_packet_del(pkt);
}

static void fort_shaper_packet_drop(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    UNUSED(shaper);

    fort_shaper_packet_free(pkt);
}

inline static PFORT_PENDING_PACKET fort_pending_packet_new(void)
{
    return fort_mem_alloc(sizeof(FORT_PENDING_PACKET), FORT_PACKET_POOL_TAG);
}

inline static void fort_pending_packet_del(PFORT_PENDING_PACKET pkt)
{
    fort_mem_free(pkt, FORT_PACKET_POOL_TAG);
}

static void fort_pending_packet_free(PFORT_PENDING_PACKET pkt)
{
    fort_packet_free(&pkt->io);

    fort_pending_packet_del(pkt);
}

static void NTAPI fort_packet_inject_complete(
        PFORT_PACKET_IO pkt, PNET_BUFFER_LIST clonedNetBufList, BOOLEAN dispatchLevel)
{
    UNUSED(clonedNetBufList);
    UNUSED(dispatchLevel);

    FORT_CHECK_STACK(FORT_PACKET_INJECT_COMPLETE);

    switch (pkt->flags & FORT_PACKET_TYPE_MASK) {
    case FORT_PACKET_TYPE_FLOW: {
        fort_shaper_packet_free((PFORT_FLOW_PACKET) pkt);
    } break;
    case FORT_PACKET_TYPE_PENDING: {
        fort_pending_packet_free((PFORT_PENDING_PACKET) pkt);
    } break;
    }
}

static NTSTATUS fort_packet_inject_in(
        const PFORT_PACKET_IO pkt, HANDLE injection_id, ADDRESS_FAMILY addressFamily)
{
    const PFORT_PACKET_IN pkt_in = &pkt->in;

    return FwpsInjectTransportReceiveAsync0(injection_id, NULL, NULL, 0, addressFamily,
            pkt->compartmentId, pkt_in->interfaceIndex, pkt_in->subInterfaceIndex, pkt->netBufList,
            (FWPS_INJECT_COMPLETE0) &fort_packet_inject_complete, pkt);
}

static NTSTATUS fort_packet_inject_out(
        const PFORT_PACKET_IO pkt, HANDLE injection_id, ADDRESS_FAMILY addressFamily)
{
    PFORT_PACKET_OUT pkt_out = &pkt->out;

    FWPS_TRANSPORT_SEND_PARAMS0 sendArgs;
    RtlZeroMemory(&sendArgs, sizeof(FWPS_TRANSPORT_SEND_PARAMS0));

    sendArgs.remoteAddress = (UCHAR *) &pkt_out->remoteAddr;
    sendArgs.remoteScopeId = pkt_out->remoteScopeId;
    sendArgs.controlData = pkt_out->controlData;
    sendArgs.controlDataLength = pkt_out->controlDataLength;

    return FwpsInjectTransportSendAsync0(injection_id, NULL, pkt_out->endpointHandle, 0, &sendArgs,
            addressFamily, pkt->compartmentId, pkt->netBufList,
            (FWPS_INJECT_COMPLETE0) &fort_packet_inject_complete, pkt);
}

static NTSTATUS fort_packet_clone(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IO pkt)
{
    NTSTATUS status;

    ULONG bytesRetreated = 0;
    if (ca->inbound) {
        bytesRetreated = ca->inMetaValues->transportHeaderSize + ca->inMetaValues->ipHeaderSize;
    }

    if (bytesRetreated != 0) {
        status = NdisRetreatNetBufferDataStart(
                NET_BUFFER_LIST_FIRST_NB(ca->netBufList), bytesRetreated, 0, 0);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = FwpsAllocateCloneNetBufferList0(ca->netBufList, NULL, NULL, 0, &pkt->netBufList);

    if (bytesRetreated != 0) {
        NdisAdvanceNetBufferDataStart(
                NET_BUFFER_LIST_FIRST_NB(ca->netBufList), bytesRetreated, FALSE, 0);
    }

    return status;
}

static NTSTATUS fort_packet_inject(PFORT_PACKET_IO pkt)
{
    NTSTATUS status;

    const BOOL inbound = (pkt->flags & FORT_PACKET_INBOUND) != 0;
    const BOOL isIPv6 = (pkt->flags & FORT_PACKET_IP6) != 0;
    const ADDRESS_FAMILY addressFamily = (isIPv6 ? AF_INET6 : AF_INET);
    const HANDLE injection_id = fort_packet_injection_id(isIPv6, inbound);

    status = inbound ? fort_packet_inject_in(pkt, injection_id, addressFamily)
                     : fort_packet_inject_out(pkt, injection_id, addressFamily);

    if (!NT_SUCCESS(status)) {
        LOG("Shaper: Packet injection call error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_INJECTION_CALL_ERROR, status, 0, 0);
    }

    return status;
}

inline static void fort_packet_fill_in_interface_indexes(
        PCFORT_CALLOUT_ARG ca, int *interfaceField, int *subInterfaceField)
{
    switch (ca->inFixedValues->layerId) {
    case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
        *interfaceField = FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_ALE_AUTH_CONNECT_V4_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
        *interfaceField = FWPS_FIELD_ALE_AUTH_CONNECT_V6_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_ALE_AUTH_CONNECT_V6_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
        *interfaceField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
        *interfaceField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_INBOUND_TRANSPORT_V4:
        *interfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_INBOUND_TRANSPORT_V6:
        *interfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V6_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V6_SUB_INTERFACE_INDEX;
        break;
    default:
        assert(0);
    }
}

inline static NTSTATUS fort_packet_fill_in(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IN pkt_in)
{
    int interfaceField;
    int subInterfaceField;
    fort_packet_fill_in_interface_indexes(ca, &interfaceField, &subInterfaceField);

    pkt_in->interfaceIndex = ca->inFixedValues->incomingValue[interfaceField].value.uint32;
    pkt_in->subInterfaceIndex = ca->inFixedValues->incomingValue[subInterfaceField].value.uint32;

    return STATUS_SUCCESS;
}

inline static void fort_packet_fill_out_remoteIp_index(PCFORT_CALLOUT_ARG ca, int *remoteIpField)
{
    switch (ca->inFixedValues->layerId) {
    case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
        *remoteIpField = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
        *remoteIpField = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
        *remoteIpField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
        *remoteIpField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
        *remoteIpField = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
        *remoteIpField = FWPS_FIELD_OUTBOUND_TRANSPORT_V6_IP_REMOTE_ADDRESS;
        break;
    default:
        NT_ASSERT(0);
    }
}

inline static NTSTATUS fort_packet_fill_out_controlData(
        PCFORT_CALLOUT_ARG ca, PFORT_PACKET_OUT pkt_out)
{
    if (!FWPS_IS_METADATA_FIELD_PRESENT(
                ca->inMetaValues, FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA))
        return STATUS_SUCCESS;

    const ULONG controlDataLength = ca->inMetaValues->controlDataLength;
    if (controlDataLength == 0)
        return STATUS_SUCCESS;

    pkt_out->controlData = fort_mem_alloc(controlDataLength, FORT_PACKET_POOL_TAG);
    if (pkt_out->controlData == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(pkt_out->controlData, ca->inMetaValues->controlData, controlDataLength);

    pkt_out->controlDataLength = controlDataLength;

    return STATUS_SUCCESS;
}

inline static NTSTATUS fort_packet_fill_out(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_OUT pkt_out)
{
    const NTSTATUS status = fort_packet_fill_out_controlData(ca, pkt_out);
    if (!NT_SUCCESS(status))
        return status;

    pkt_out->remoteScopeId = ca->inMetaValues->remoteScopeId;
    pkt_out->endpointHandle = ca->inMetaValues->transportEndpointHandle;

    int remoteIpField;
    fort_packet_fill_out_remoteIp_index(ca, &remoteIpField);

    const FWP_VALUE0 *remoteIpValue = &ca->inFixedValues->incomingValue[remoteIpField].value;
    if (ca->isIPv6) {
        pkt_out->remoteAddr.v6 = *((ip6_addr_t *) remoteIpValue->byteArray16);
    } else {
        /* host-order -> network-order conversion */
        pkt_out->remoteAddr.v4 = HTONL(remoteIpValue->uint32);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS fort_packet_fill(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IO pkt, UCHAR pkt_flags)
{
    NTSTATUS status;

    status = ca->inbound ? fort_packet_fill_in(ca, &pkt->in) : fort_packet_fill_out(ca, &pkt->out);

    if (!NT_SUCCESS(status))
        return status;

    pkt->flags = (ca->inbound ? FORT_PACKET_INBOUND : 0) | (ca->isIPv6 ? FORT_PACKET_IP6 : 0)
            | pkt_flags;

    pkt->compartmentId = ca->inMetaValues->compartmentId;

    status = fort_packet_clone(ca, pkt);

    if (!NT_SUCCESS(status)) {
        LOG("Shaper: Packet clone error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_CLONE_ERROR, status, 0, 0);
    }

    return status;
}

static void fort_shaper_packet_inject(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    NTSTATUS status;

    status = fort_packet_inject(&pkt->io);

    if (!NT_SUCCESS(status)) {
        fort_shaper_packet_free(pkt);
    }
}

static void fort_shaper_packet_foreach(
        PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt, FORT_SHAPER_PACKET_FOREACH_FUNC *func)
{
    while (pkt != NULL) {
        PFORT_FLOW_PACKET pkt_next = pkt->next;

        func(shaper, pkt);

        pkt = pkt_next;
    }
}

inline static BOOL fort_shaper_packet_list_is_empty(PFORT_PACKET_LIST pkt_list)
{
    return (pkt_list->packet_head == NULL);
}

static void fort_shaper_packet_list_add_chain(
        PFORT_PACKET_LIST pkt_list, PFORT_FLOW_PACKET pkt_head, PFORT_FLOW_PACKET pkt_tail)
{
    if (pkt_list->packet_tail == NULL) {
        pkt_list->packet_head = pkt_head;
    } else {
        pkt_list->packet_tail->next = pkt_head;
    }

    pkt_list->packet_tail = pkt_tail;
}

static PFORT_FLOW_PACKET fort_shaper_packet_list_get(
        PFORT_PACKET_LIST pkt_list, PFORT_FLOW_PACKET pkt)
{
    if (pkt_list->packet_head != NULL) {
        pkt_list->packet_tail->next = pkt;
        pkt = pkt_list->packet_head;

        pkt_list->packet_head = pkt_list->packet_tail = NULL;
    }

    return pkt;
}

static void fort_shaper_packet_list_cut_chain(PFORT_PACKET_LIST pkt_list, PFORT_FLOW_PACKET pkt)
{
    pkt_list->packet_head = pkt->next;
    pkt->next = NULL;

    if (pkt_list->packet_head == NULL) {
        pkt_list->packet_tail = NULL;
    }
}

static void fort_shaper_packet_list_cut_packet(PFORT_PACKET_LIST pkt_list, PFORT_FLOW_PACKET pkt,
        PFORT_FLOW_PACKET pkt_prev, PFORT_FLOW_PACKET pkt_next)
{
    if (pkt_prev != NULL) {
        pkt_prev->next = pkt_next;
    } else {
        pkt_list->packet_head = pkt_next;
    }

    if (pkt_next == NULL) {
        pkt_list->packet_tail = pkt_prev;
    }
}

static PFORT_FLOW_PACKET fort_shaper_packet_list_get_flow_packets(
        PFORT_PACKET_LIST pkt_list, PFORT_FLOW flow, PFORT_FLOW_PACKET pkt_chain)
{
    PFORT_FLOW_PACKET pkt_prev = NULL;
    PFORT_FLOW_PACKET pkt = pkt_list->packet_head;

    while (pkt != NULL) {
        PFORT_FLOW_PACKET pkt_next = pkt->next;

        if (pkt->flow == flow) {
            fort_shaper_packet_list_cut_packet(pkt_list, pkt, pkt_prev, pkt_next);

            pkt->next = pkt_chain;
            pkt_chain = pkt;
        } else {
            pkt_prev = pkt;
        }

        pkt = pkt_next;
    }

    return pkt_chain;
}

static void fort_shaper_queue_advance_available(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, const LARGE_INTEGER now)
{
    const LARGE_INTEGER last_tick = queue->last_tick;
    queue->last_tick = now;

    const UINT64 bps = queue->limit.bps;

    /* Advance the available bytes */
    const UINT64 accumulated =
            ((now.QuadPart - last_tick.QuadPart) * bps) / shaper->qpcFrequency.QuadPart;

    queue->available_bytes += accumulated;

    const UINT64 max_available = bps;
    if (queue->available_bytes > max_available) {
        queue->available_bytes = max_available;
    }

    /*
    LOG("Shaper: BAND: queued=%d avail=%d ms=%d\n", (UINT32) queue->queued_bytes,
            (UINT32) queue->available_bytes,
            (UINT32) (((now.QuadPart - last_tick.QuadPart) * 1000)
                    / shaper->qpcFrequency.QuadPart));
    */
}

static void fort_shaper_queue_process_bandwidth(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, const LARGE_INTEGER now)
{
    /* Move packets to the latency queue as the accumulated available bytes will allow */
    PFORT_FLOW_PACKET pkt_chain = queue->bandwidth_list.packet_head;
    if (pkt_chain == NULL)
        return;

    PFORT_FLOW_PACKET pkt_tail = NULL;
    PFORT_FLOW_PACKET pkt = pkt_chain;
    do {
        const UINT64 pkt_length = pkt->data_length;

        if (queue->available_bytes < pkt_length)
            break;

        queue->available_bytes -= pkt_length;
        queue->queued_bytes -= pkt_length;

        pkt->latency_start = now;

        pkt_tail = pkt;
        pkt = pkt->next;
    } while (pkt != NULL);

    if (pkt_tail != NULL) {
        fort_shaper_packet_list_cut_chain(&queue->bandwidth_list, pkt_tail);

        fort_shaper_packet_list_add_chain(&queue->latency_list, pkt_chain, pkt_tail);
    }
}

static PFORT_FLOW_PACKET fort_shaper_queue_process_latency(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, const LARGE_INTEGER now)
{
    PFORT_FLOW_PACKET pkt_chain = queue->latency_list.packet_head;
    if (pkt_chain == NULL)
        return NULL;

    const UINT32 latency_ms = queue->limit.latency_ms;

    if (latency_ms == 0) {
        fort_shaper_packet_list_cut_chain(&queue->latency_list, queue->latency_list.packet_tail);

        return pkt_chain;
    }

    const UINT64 qpcFrequency = shaper->qpcFrequency.QuadPart;
    const UINT64 qpcFrequencyHalfMs = qpcFrequency / 2000LL;

    PFORT_FLOW_PACKET pkt_tail = NULL;
    PFORT_FLOW_PACKET pkt = pkt_chain;
    do {
        /* Round to the closest ms instead of truncating
         * by adding 1/2 of a ms to the elapsed ticks */
        const ULONG elapsed_ms = (ULONG) (((now.QuadPart - pkt->latency_start.QuadPart) * 1000LL
                                                  + qpcFrequencyHalfMs)
                / qpcFrequency);

        if (elapsed_ms < latency_ms)
            break;

        pkt_tail = pkt;
        pkt = pkt->next;
    } while (pkt != NULL);

    if (pkt_tail != NULL) {
        fort_shaper_packet_list_cut_chain(&queue->latency_list, pkt_tail);

        return pkt_chain;
    }

    return NULL;
}

static PFORT_FLOW_PACKET fort_shaper_queue_get_packets(
        PFORT_PACKET_QUEUE queue, PFORT_FLOW_PACKET pkt)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);

    queue->queued_bytes = 0;

    pkt = fort_shaper_packet_list_get(&queue->latency_list, pkt);
    pkt = fort_shaper_packet_list_get(&queue->bandwidth_list, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pkt;
}

static PFORT_FLOW_PACKET fort_shaper_queue_get_flow_packets(
        PFORT_PACKET_QUEUE queue, PFORT_FLOW flow, PFORT_FLOW_PACKET pkt)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);

    pkt = fort_shaper_packet_list_get_flow_packets(&queue->bandwidth_list, flow, pkt);
    pkt = fort_shaper_packet_list_get_flow_packets(&queue->latency_list, flow, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pkt;
}

inline static BOOL fort_shaper_queue_is_empty(PFORT_PACKET_QUEUE queue)
{
    return fort_shaper_packet_list_is_empty(&queue->bandwidth_list)
            && fort_shaper_packet_list_is_empty(&queue->latency_list);
}

static BOOL fort_shaper_queue_process(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, const LARGE_INTEGER now)
{
    PFORT_FLOW_PACKET pkt_chain = NULL;
    BOOL is_active = FALSE;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);

    if (!fort_shaper_queue_is_empty(queue)) {
        fort_shaper_queue_advance_available(shaper, queue, now);
        fort_shaper_queue_process_bandwidth(shaper, queue, now);

        pkt_chain = fort_shaper_queue_process_latency(shaper, queue, now);

        is_active = !fort_shaper_queue_is_empty(queue);
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    if (pkt_chain != NULL) {
        fort_shaper_packet_foreach(shaper, pkt_chain, &fort_shaper_packet_inject);
    }

    return is_active;
}

inline static PFORT_PACKET_QUEUE fort_shaper_create_queue(PFORT_SHAPER shaper, int queue_index)
{
    PFORT_PACKET_QUEUE queue = shaper->queues[queue_index];
    if (queue != NULL)
        return queue;

    queue = fort_mem_alloc(sizeof(FORT_PACKET_QUEUE), FORT_PACKET_POOL_TAG);
    if (queue == NULL)
        return NULL;

    RtlZeroMemory(queue, sizeof(FORT_PACKET_QUEUE));

    KeInitializeSpinLock(&queue->lock);

    shaper->queues[queue_index] = queue;

    return queue;
}

static void fort_shaper_create_queues(
        PFORT_SHAPER shaper, PFORT_SPEED_LIMIT limits, UINT32 limit_io_bits)
{
    const LARGE_INTEGER now = KeQueryPerformanceCounter(NULL);

    for (int i = 0; limit_io_bits != 0; ++i) {
        const BOOL queue_exists = (limit_io_bits & 1) != 0;
        limit_io_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = fort_shaper_create_queue(shaper, i);
        if (queue == NULL)
            continue;

        queue->limit = limits[i];

        queue->available_bytes = FORT_QUEUE_INITIAL_TOKEN_COUNT;
        queue->last_tick = now;
    }
}

static void fort_shaper_free_queues(PFORT_SHAPER shaper)
{
    for (int i = 0; i < FORT_CONF_GROUP_MAX; ++i) {
        PFORT_PACKET_QUEUE queue = shaper->queues[i];
        if (queue == NULL)
            continue;

        fort_mem_free(queue, FORT_PACKET_POOL_TAG);
    }
}

inline static void fort_shaper_thread_set_event(PFORT_SHAPER shaper)
{
    KeSetEvent(&shaper->thread_event, IO_NO_INCREMENT, FALSE);
}

inline static ULONG fort_shaper_thread_process_queues(
        PFORT_SHAPER shaper, ULONG active_io_bits, const LARGE_INTEGER now)
{
    ULONG new_active_io_bits = 0;

    for (int i = 0; active_io_bits != 0; ++i) {
        const BOOL queue_exists = (active_io_bits & 1) != 0;
        active_io_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = shaper->queues[i];
        if (queue == NULL)
            continue;

        if (fort_shaper_queue_process(shaper, queue, now)) {
            new_active_io_bits |= (1 << i);
        }
    }

    return new_active_io_bits;
}

inline static BOOL fort_shaper_thread_process(PFORT_SHAPER shaper, const LARGE_INTEGER now)
{
    ULONG active_io_bits =
            fort_shaper_io_bits_set(&shaper->active_io_bits, FORT_PACKET_FLUSH_ALL, FALSE);

    if (active_io_bits == 0)
        return FALSE;

    active_io_bits = fort_shaper_thread_process_queues(shaper, active_io_bits, now);

    if (active_io_bits != 0) {
        fort_shaper_io_bits_set(&shaper->active_io_bits, active_io_bits, TRUE);

        return TRUE;
    }

    return FALSE;
}

static void fort_shaper_thread_loop(PVOID context)
{
    PFORT_SHAPER shaper = context;
    PKEVENT thread_event = &shaper->thread_event;

    LARGE_INTEGER delay = {
        .QuadPart = -2 * 1000 * 10 /* sleep 2000us (2ms) */
    };

    PLARGE_INTEGER timeout = NULL;

    do {
        KeWaitForSingleObject(thread_event, Executive, KernelMode, FALSE, timeout);

        const LARGE_INTEGER now = KeQueryPerformanceCounter(NULL); /* get current time ASAP */

        const BOOL is_active = fort_shaper_thread_process(shaper, now);

        timeout = is_active ? &delay : NULL;

    } while ((fort_shaper_flags(shaper) & FORT_SHAPER_CLOSED) == 0);
}

static void fort_shaper_thread_close(PFORT_SHAPER shaper)
{
    fort_shaper_thread_set_event(shaper);

    fort_thread_wait(&shaper->thread);
}

inline static PFORT_FLOW_PACKET fort_shaper_flush_queues(PFORT_SHAPER shaper, UINT32 group_io_bits)
{
    PFORT_FLOW_PACKET pkt_chain = NULL;

    for (int i = 0; group_io_bits != 0; ++i) {
        const BOOL queue_exists = (group_io_bits & 1) != 0;
        group_io_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = shaper->queues[i];
        if (queue == NULL)
            continue;

        pkt_chain = fort_shaper_queue_get_packets(queue, pkt_chain);
    }

    return pkt_chain;
}

static void fort_shaper_flush(PFORT_SHAPER shaper, UINT32 group_io_bits, BOOL drop)
{
    if (group_io_bits == 0)
        return;

    group_io_bits &= fort_shaper_io_bits_set(&shaper->active_io_bits, group_io_bits, FALSE);

    /* Collect packets from Queues */
    PFORT_FLOW_PACKET pkt_chain = fort_shaper_flush_queues(shaper, group_io_bits);

    /* Process the packets */
    if (pkt_chain != NULL) {
        fort_shaper_packet_foreach(
                shaper, pkt_chain, (drop ? &fort_shaper_packet_drop : &fort_shaper_packet_inject));
    }
}

FORT_API void fort_shaper_open(PFORT_SHAPER shaper)
{
    const LARGE_INTEGER now = KeQueryPerformanceCounter(&shaper->qpcFrequency);
    shaper->randomSeed = now.LowPart;

    KeInitializeSpinLock(&shaper->lock);

    KeInitializeEvent(&shaper->thread_event, SynchronizationEvent, FALSE);

    fort_thread_run(&shaper->thread, &fort_shaper_thread_loop, shaper, /*priorityIncrement=*/0);
}

FORT_API void fort_shaper_close(PFORT_SHAPER shaper)
{
    fort_shaper_flags_set(shaper, FORT_SHAPER_CLOSED, TRUE);

    fort_shaper_thread_close(shaper);

    fort_shaper_drop_packets(shaper);
    fort_shaper_free_queues(shaper);
}

FORT_API void fort_shaper_conf_update(PFORT_SHAPER shaper, const PFORT_CONF_IO conf_io)
{
    const PFORT_CONF_GROUP conf_group = &conf_io->conf_group;
    const PFORT_CONF_FLAGS conf_flags = &conf_io->conf.flags;

    const UINT32 limit_io_bits = conf_group->limit_io_bits;
    const UINT32 group_io_bits = conf_flags->filter_enabled
            ? (limit_io_bits & fort_bits_duplicate16(conf_group->group_bits))
            : 0;
    UINT32 flush_io_bits;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);
    {
        flush_io_bits = (limit_io_bits ^ shaper->group_io_bits);

        fort_shaper_create_queues(shaper, conf_group->limits, limit_io_bits);

        shaper->limit_io_bits = limit_io_bits;

        fort_shaper_io_bits_exchange(&shaper->group_io_bits, group_io_bits);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_shaper_flush(shaper, flush_io_bits, /*drop=*/FALSE);
}

void fort_shaper_conf_flags_update(PFORT_SHAPER shaper, const PFORT_CONF_FLAGS conf_flags)
{
    const UINT32 group_io_bits =
            conf_flags->filter_enabled ? fort_bits_duplicate16((UINT16) conf_flags->group_bits) : 0;
    UINT32 flush_io_bits;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);
    {
        flush_io_bits = (group_io_bits ^ shaper->group_io_bits);

        fort_shaper_io_bits_exchange(
                &shaper->group_io_bits, (shaper->limit_io_bits & group_io_bits));
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_shaper_flush(shaper, flush_io_bits, /*drop=*/FALSE);
}

static void fort_shaper_packet_queue_add_packet(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, PFORT_FLOW_PACKET pkt, UINT32 queue_bit)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);
    {
        queue->queued_bytes += pkt->data_length;

        fort_shaper_packet_list_add_chain(&queue->bandwidth_list, pkt, pkt);

        fort_shaper_io_bits_set(&shaper->active_io_bits, queue_bit, TRUE);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

inline static BOOL fort_shaper_packet_queue_check_plr(PFORT_PACKET_QUEUE queue)
{
    const UINT16 plr = queue->limit.plr;
    if (plr > 0) {
        PFORT_SHAPER shaper = &fort_device()->shaper;

        const ULONG random = RtlRandomEx(&shaper->randomSeed) % 10000; /* PLR range is 0-10000 */
        if (random < plr)
            return FALSE;
    }
    return TRUE;
}

inline static BOOL fort_shaper_packet_queue_check_buffer(
        PFORT_PACKET_QUEUE queue, ULONG data_length)
{
    const UINT32 buffer_bytes = queue->limit.buffer_bytes;

    return buffer_bytes == 0 || (UINT64) buffer_bytes >= (queue->queued_bytes + data_length);
}

static BOOL fort_shaper_packet_queue_check_packet(PFORT_PACKET_QUEUE queue, ULONG data_length)
{
    BOOL res;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);
    {
        res = fort_shaper_packet_queue_check_plr(queue)
                && fort_shaper_packet_queue_check_buffer(queue, data_length);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return res;
}

inline static NTSTATUS fort_shaper_packet_queue(
        PFORT_SHAPER shaper, PCFORT_CALLOUT_ARG ca, PFORT_FLOW flow)
{
    const UINT16 queue_index = flow->opt.group_index * 2 + (ca->inbound ? 0 : 1);

    const UINT32 group_io_bits = fort_shaper_io_bits(&shaper->group_io_bits);

    const UINT32 queue_bit = (1 << queue_index);
    if ((group_io_bits & queue_bit) == 0)
        return STATUS_NO_SUCH_GROUP;

    PFORT_PACKET_QUEUE queue = shaper->queues[queue_index];
    if (queue == NULL)
        return STATUS_NO_SUCH_GROUP;

    /* Calculate the Packets' Data Length */
    const ULONG data_length = fort_packet_data_length(ca);

    /* Check the Queue for new Packet */
    if (!fort_shaper_packet_queue_check_packet(queue, data_length)) {
        return STATUS_SUCCESS; /* drop the packet */
    }

    /* Create the Packet */
    PFORT_FLOW_PACKET pkt = fort_shaper_packet_new();
    if (pkt == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(pkt, sizeof(FORT_FLOW_PACKET));

    const NTSTATUS status = fort_packet_fill(ca, &pkt->io, FORT_PACKET_TYPE_FLOW);
    if (!NT_SUCCESS(status)) {
        fort_shaper_packet_free(pkt);
        return status;
    }

    pkt->flow = flow;
    pkt->data_length = data_length;

    /* Add the Packet to Queue */
    fort_shaper_packet_queue_add_packet(shaper, queue, pkt, queue_bit);

    /* Packets in transport layer must be re-injected in DCP/thread due to locking */
    fort_shaper_thread_set_event(shaper);

    return STATUS_SUCCESS;
}

FORT_API BOOL fort_shaper_packet_process(PFORT_SHAPER shaper, PFORT_CALLOUT_ARG ca)
{
    if (fort_packet_is_ipsec_tunneled(ca)) {
        /* To be compatible with Vista's IpSec implementation, we must not
         * intercept not-yet-detunneled IpSec traffic. */
        return FALSE;
    }

    PFORT_FLOW flow = (PFORT_FLOW) ca->flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);
    const UCHAR speed_limit = ca->inbound ? FORT_FLOW_SPEED_LIMIT_IN : FORT_FLOW_SPEED_LIMIT_OUT;

    if ((flow_flags & speed_limit) == 0
            || fort_device_flag(&fort_device()->conf, FORT_DEVICE_POWER_OFF) != 0)
        return FALSE;

    ca->isIPv6 = (flow_flags & FORT_FLOW_IP6) != 0;

    /* Skip self injected packet */
    if (fort_packet_injected_by_self(ca))
        return FALSE;

    const NTSTATUS status = fort_shaper_packet_queue(&fort_device()->shaper, ca, flow);

    return NT_SUCCESS(status);
}

FORT_API void fort_shaper_drop_flow_packets(PFORT_SHAPER shaper, UINT64 flowContext)
{
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);
    const UCHAR speed_limit = (flow_flags & FORT_FLOW_SPEED_LIMIT_FLAGS);

    if (speed_limit == 0)
        return;

    /* Collect flow's packets from Queues */
    PFORT_FLOW_PACKET pkt_chain = NULL;

    UINT32 active_io_bits = fort_shaper_io_bits(&shaper->active_io_bits)
            & (speed_limit << (flow->opt.group_index * 2));

    for (int i = 0; active_io_bits != 0; ++i) {
        const BOOL queue_exists = (active_io_bits & 1) != 0;
        active_io_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = shaper->queues[i];
        if (queue == NULL)
            continue;

        pkt_chain = fort_shaper_queue_get_flow_packets(queue, flow, pkt_chain);
    }

    /* Drop the packets */
    if (pkt_chain != NULL) {
        fort_shaper_packet_foreach(shaper, pkt_chain, &fort_shaper_packet_drop);
    }
}

FORT_API void fort_shaper_drop_packets(PFORT_SHAPER shaper)
{
    fort_shaper_flush(shaper, FORT_PACKET_FLUSH_ALL, /*drop=*/TRUE);
}

static PFORT_PENDING_PROC fort_pending_proc_find_locked(PFORT_PENDING pending, UINT32 process_id)
{
    PFORT_PENDING_PROC proc = pending->procs_head;

    for (; proc != NULL; proc = proc->next) {
        if (proc->process_id == process_id)
            return proc;
    }

    return NULL;
}

static BOOL fort_pending_proc_check_limits(PFORT_PENDING pending, UINT32 process_id)
{
    UINT16 proc_count = 0;
    UINT16 packet_count = 0;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    proc_count = pending->proc_count;

    PFORT_PENDING_PROC proc = fort_pending_proc_find_locked(pending, process_id);
    if (proc != NULL) {
        packet_count = proc->packet_count;
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return proc_count < FORT_PENDING_PROC_COUNT_MAX
            && packet_count < FORT_PENDING_PROC_PACKET_COUNT_MAX;
}

static PFORT_PENDING_PROC fort_pending_proc_get_locked(PFORT_PENDING pending, UINT32 process_id)
{
    PFORT_PENDING_PROC proc;

    if (pending->proc_free != NULL) {
        proc = pending->proc_free;
        pending->proc_free = proc->next;
    } else {
        const tommy_size_t size = tommy_arrayof_size(&pending->procs);

        /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
        if (tommy_arrayof_grow(&pending->procs, size + 1), 0)
            return NULL;

        proc = tommy_arrayof_ref(&pending->procs, size);
    }

    proc->packets_head = NULL;
    proc->packet_count = 0;
    proc->process_id = process_id;

    proc->next = pending->procs_head;
    pending->procs_head = proc;

    pending->proc_count++;

    return proc;
}

static PFORT_PENDING_PROC fort_pending_proc_get_check(PFORT_PENDING pending, UINT32 process_id)
{
    PFORT_PENDING_PROC proc = fort_pending_proc_find_locked(pending, process_id);

    if (proc == NULL) {
        return fort_pending_proc_get_locked(pending, process_id);
    }

    if (proc->packet_count >= FORT_PENDING_PROC_PACKET_COUNT_MAX)
        return NULL;

    return proc;
}

static void fort_pending_proc_put_locked(PFORT_PENDING pending, PFORT_PENDING_PROC proc)
{
    pending->proc_count--;

    proc->next = pending->proc_free;
    pending->proc_free = proc;
}

static NTSTATUS fort_pending_proc_add_packet_locked(PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_PENDING_PACKET pkt)
{
    /* Create the Pending Process */
    PFORT_PENDING_PROC proc = fort_pending_proc_get_check(pending, cx->process_id);
    if (proc == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    const NTSTATUS status =
            FwpsPendOperation0(ca->inMetaValues->completionHandle, &pkt->completion_context);

    if (!NT_SUCCESS(status)) {
        if (proc->packet_count == 0) {
            fort_pending_proc_put_locked(pending, proc);
        }
        return status;
    }

    proc->packet_count++;

    pkt->next = proc->packets_head;
    proc->packets_head = pkt;

    return STATUS_SUCCESS;
}

static NTSTATUS fort_pending_proc_add_packet(PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_PENDING_PACKET pkt)
{
    NTSTATUS status;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    status = fort_pending_proc_add_packet_locked(pending, ca, cx, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return status;
}

static void fort_pending_init(PFORT_PENDING pending)
{
    tommy_arrayof_init(&pending->procs, sizeof(FORT_PENDING_PROC));
}

FORT_API void fort_pending_open(PFORT_PENDING pending)
{
    FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport4_in_id);
    FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport4_out_id);
    FwpsInjectionHandleCreate0(
            AF_INET6, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport6_in_id);
    FwpsInjectionHandleCreate0(
            AF_INET6, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport6_out_id);

    fort_pending_init(pending);

    KeInitializeSpinLock(&pending->lock);
}

static void fort_pending_done(PFORT_PENDING pending)
{
    tommy_arrayof_done(&pending->procs);
}

FORT_API void fort_pending_close(PFORT_PENDING pending)
{
    fort_pending_done(pending);

    FwpsInjectionHandleDestroy0(pending->injection_transport4_in_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport4_out_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport6_in_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport6_out_id);
}

static void fort_pending_clear_locked(PFORT_PENDING pending)
{
    if (pending->proc_count == 0)
        return;

    pending->proc_count = 0;
    pending->proc_free = NULL;
    pending->procs_head = NULL;

    fort_pending_done(pending);
    fort_pending_init(pending);
}

FORT_API void fort_pending_clear(PFORT_PENDING pending)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    fort_pending_clear_locked(pending);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API BOOL fort_pending_add_packet(
        PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx)
{
    NTSTATUS status;

    /* Skip self injected packet */
    if (fort_packet_injected_by_self(ca))
        return FALSE;

    /* Check the Process's Limits */
    if (!fort_pending_proc_check_limits(pending, cx->process_id))
        return FALSE;

    /* Create the Packet */
    PFORT_PENDING_PACKET pkt = fort_pending_packet_new();
    if (pkt == NULL)
        return FALSE;

    RtlZeroMemory(pkt, sizeof(FORT_PENDING_PACKET));

    const UCHAR ipsec_flag = fort_packet_is_ipsec_protected(ca) ? FORT_PACKET_IPSEC_PROTECTED : 0;

    status = fort_packet_fill(ca, &pkt->io, ipsec_flag | FORT_PACKET_TYPE_PENDING);
    if (NT_SUCCESS(status)) {
        /* Add the Packet to Pending Process */
        status = fort_pending_proc_add_packet(pending, ca, cx, pkt);
    }

    if (!NT_SUCCESS(status)) {
        fort_pending_packet_free(pkt);
        return FALSE;
    }

    return TRUE;
}
