/* Fort Firewall Packets Shaping & Re-injection */

#include "fortpkt.h"

#include "fortdev.h"
#include "forttrace.h"
#include "fortutl.h"

#define FORT_PACKET_POOL_TAG 'KwfF'

#define FORT_PACKET_FLUSH_ALL 0xFFFFFFFF

#define FORT_QUEUE_INITIAL_TOKEN_COUNT 1500

#define HTONL(l) _byteswap_ulong(l)

typedef void FORT_SHAPER_PACKET_FOREACH_FUNC(PFORT_SHAPER, PFORT_FLOW_PACKET);

static LARGE_INTEGER g_QpcFrequency;
static UINT64 g_QpcFrequencyHalfMs;
static ULONG g_RandomSeed;

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

static BOOL fort_packet_injected_by_self(HANDLE injection_id, PNET_BUFFER_LIST netBufList)
{
    const FWPS_PACKET_INJECTION_STATE state =
            FwpsQueryPacketInjectionState0(injection_id, netBufList, NULL);

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

static ULONG fort_packet_data_length(const PNET_BUFFER_LIST netBufList)
{
    ULONG data_length = 0;

    PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList);
    while (netBuf != NULL) {
        data_length += NET_BUFFER_DATA_LENGTH(netBuf);
        netBuf = NET_BUFFER_NEXT_NB(netBuf);
    }

    return data_length;
}

static PFORT_FLOW_PACKET fort_shaper_packet_get_locked(PFORT_SHAPER shaper)
{
    PFORT_FLOW_PACKET pkt = NULL;

    if (shaper->packet_free != NULL) {
        pkt = shaper->packet_free;
        shaper->packet_free = pkt->next;
    } else {
        const tommy_size_t size = tommy_arrayof_size(&shaper->packets);

        /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
        if (tommy_arrayof_grow(&shaper->packets, size + 1), 0)
            return NULL;

        pkt = tommy_arrayof_ref(&shaper->packets, size);
    }

    return pkt;
}

static PFORT_FLOW_PACKET fort_shaper_packet_get(PFORT_SHAPER shaper)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);

    PFORT_FLOW_PACKET pkt = fort_shaper_packet_get_locked(shaper);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pkt;
}

static void fort_shaper_packet_put_locked(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    pkt->next = shaper->packet_free;
    shaper->packet_free = pkt;
}

static void fort_shaper_packet_put(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);

    fort_shaper_packet_put_locked(shaper, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
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
        NT_ASSERT(0);
    }
}

inline static NTSTATUS fort_packet_fill_in(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IN pkt_in)
{
    int interfaceField;
    int subInterfaceField;
    fort_packet_fill_in_interface_indexes(ca, &interfaceField, &subInterfaceField);

    pkt_in->interfaceIndex = ca->inFixedValues->incomingValue[interfaceField].value.uint32;
    pkt_in->subInterfaceIndex = ca->inFixedValues->incomingValue[subInterfaceField].value.uint32;

    pkt_in->bytesRetreated = ca->inMetaValues->transportHeaderSize + ca->inMetaValues->ipHeaderSize;

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

inline static NTSTATUS fort_packet_fill_out(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_OUT pkt_out)
{
    const ULONG controlDataLength = ca->inMetaValues->controlDataLength;
    if (FWPS_IS_METADATA_FIELD_PRESENT(ca->inMetaValues, FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA)
            && controlDataLength > 0) {
        pkt_out->controlData = fort_mem_alloc(controlDataLength, FORT_PACKET_POOL_TAG);
        if (pkt_out->controlData == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(pkt_out->controlData, ca->inMetaValues->controlData, controlDataLength);

        pkt_out->controlDataLength = controlDataLength;
    }

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

inline static NTSTATUS fort_packet_fill(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IO pkt)
{
    const NTSTATUS status =
            ca->inbound ? fort_packet_fill_in(ca, &pkt->in) : fort_packet_fill_out(ca, &pkt->out);

    if (!NT_SUCCESS(status))
        return status;

    pkt->flags = (ca->inbound ? FORT_PACKET_INBOUND : 0) | (ca->isIPv6 ? FORT_PACKET_IP6 : 0);

    pkt->compartmentId = ca->inMetaValues->compartmentId;
    pkt->netBufList = ca->netBufList;

    if (pkt->netBufList != NULL) {
        FwpsReferenceNetBufferList0(pkt->netBufList, TRUE);
    }

    return STATUS_SUCCESS;
}

inline static void fort_shaper_packet_free_cloned(PNET_BUFFER_LIST clonedNetBufList)
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

inline static void fort_shaper_packet_free_base(PFORT_PACKET_IO pkt)
{
    if ((pkt->flags & FORT_PACKET_INBOUND) == 0) {
        if (pkt->out.controlData != NULL) {
            fort_mem_free(pkt->out.controlData, FORT_PACKET_POOL_TAG);
        }
    }

    if (pkt->netBufList != NULL) {
        FwpsDereferenceNetBufferList0(pkt->netBufList, FALSE);
    }
}

static void fort_shaper_packet_free(
        PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt, PNET_BUFFER_LIST clonedNetBufList)
{
    fort_shaper_packet_free_cloned(clonedNetBufList);
    fort_shaper_packet_free_base(&pkt->io);

    fort_shaper_packet_put(shaper, pkt);
}

static void fort_shaper_packet_drop(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    fort_shaper_packet_free(shaper, pkt, /*clonedNetBufList=*/NULL);
}

static void NTAPI fort_packet_inject_complete(
        PFORT_FLOW_PACKET pkt, PNET_BUFFER_LIST clonedNetBufList, BOOLEAN dispatchLevel)
{
    UNUSED(dispatchLevel);

    fort_shaper_packet_free(&fort_device()->shaper, pkt, clonedNetBufList);
}

static NTSTATUS fort_packet_inject_in(const PFORT_PACKET_IO pkt, PNET_BUFFER_LIST clonedNetBufList,
        HANDLE injection_id, ADDRESS_FAMILY addressFamily)
{
    const PFORT_PACKET_IN pkt_in = &pkt->in;

    return FwpsInjectTransportReceiveAsync0(injection_id, NULL, NULL, 0, addressFamily,
            pkt->compartmentId, pkt_in->interfaceIndex, pkt_in->subInterfaceIndex, clonedNetBufList,
            (FWPS_INJECT_COMPLETE0) &fort_packet_inject_complete, pkt);
}

static NTSTATUS fort_packet_inject_out(const PFORT_PACKET_IO pkt, PNET_BUFFER_LIST clonedNetBufList,
        HANDLE injection_id, ADDRESS_FAMILY addressFamily)
{
    PFORT_PACKET_OUT pkt_out = &pkt->out;

    FWPS_TRANSPORT_SEND_PARAMS0 sendArgs;
    RtlZeroMemory(&sendArgs, sizeof(FWPS_TRANSPORT_SEND_PARAMS0));

    sendArgs.remoteAddress = (UCHAR *) &pkt_out->remoteAddr;
    sendArgs.remoteScopeId = pkt_out->remoteScopeId;

    return FwpsInjectTransportSendAsync0(injection_id, NULL, pkt_out->endpointHandle, 0, &sendArgs,
            addressFamily, pkt->compartmentId, clonedNetBufList,
            (FWPS_INJECT_COMPLETE0) &fort_packet_inject_complete, pkt);
}

inline static HANDLE fort_shaper_injection_id(PFORT_SHAPER shaper, BOOL isIPv6)
{
    return isIPv6 ? shaper->injection_transport6_id : shaper->injection_transport4_id;
}

inline static NTSTATUS fort_packet_clone(
        PFORT_PACKET_IO pkt, PNET_BUFFER_LIST *clonedNetBufList, BOOL inbound)
{
    NTSTATUS status;

    const ULONG bytesRetreated = inbound ? pkt->in.bytesRetreated : 0;

    if (bytesRetreated != 0) {
        status = NdisRetreatNetBufferDataStart(
                NET_BUFFER_LIST_FIRST_NB(pkt->netBufList), bytesRetreated, 0, 0);
        if (!NT_SUCCESS(status))
            return status;
    }

    status = FwpsAllocateCloneNetBufferList0(pkt->netBufList, NULL, NULL, 0, clonedNetBufList);

    if (bytesRetreated != 0) {
        NdisAdvanceNetBufferDataStart(
                NET_BUFFER_LIST_FIRST_NB(pkt->netBufList), bytesRetreated, FALSE, 0);
    }

    return status;
}

static NTSTATUS fort_shaper_packet_base_inject(
        PFORT_SHAPER shaper, PFORT_PACKET_IO pkt, PNET_BUFFER_LIST *clonedNetBufList)
{
    NTSTATUS status;

    const BOOL inbound = (pkt->flags & FORT_PACKET_INBOUND) != 0;

    status = fort_packet_clone(pkt, clonedNetBufList, inbound);
    if (!NT_SUCCESS(status)) {
        LOG("Shaper: Packet clone error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_CLONE_ERROR, status, 0, 0);
        return status;
    }

    const BOOL isIPv6 = (pkt->flags & FORT_PACKET_IP6) != 0;
    const ADDRESS_FAMILY addressFamily = (isIPv6 ? AF_INET6 : AF_INET);
    const HANDLE injection_id = fort_shaper_injection_id(shaper, isIPv6);

    status = inbound ? fort_packet_inject_in(pkt, *clonedNetBufList, injection_id, addressFamily)
                     : fort_packet_inject_out(pkt, *clonedNetBufList, injection_id, addressFamily);
    if (!NT_SUCCESS(status)) {
        LOG("Shaper: Packet injection call error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_INJECTION_CALL_ERROR, status, 0, 0);

        (*clonedNetBufList)->Status = STATUS_SUCCESS;
    }

    return status;
}

static void fort_shaper_packet_inject(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    NTSTATUS status;

    PNET_BUFFER_LIST clonedNetBufList = NULL;

    status = fort_shaper_packet_base_inject(shaper, &pkt->io, &clonedNetBufList);

    if (!NT_SUCCESS(status)) {
        fort_shaper_packet_free(shaper, pkt, clonedNetBufList);
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

inline static void fort_shaper_packet_list_add(PFORT_PACKET_LIST pkt_list, PFORT_FLOW_PACKET pkt)
{
    fort_shaper_packet_list_add_chain(pkt_list, pkt, pkt);
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

static void fort_shaper_queue_process_bandwidth(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, const LARGE_INTEGER now)
{
    const UINT64 bps = queue->limit.bps;

    /* Advance the available bytes */
    const UINT64 accumulated =
            ((now.QuadPart - queue->last_tick.QuadPart) * bps) / g_QpcFrequency.QuadPart;
    queue->available_bytes += accumulated;

    if (fort_shaper_packet_list_is_empty(&queue->bandwidth_list)
            && queue->available_bytes > FORT_QUEUE_INITIAL_TOKEN_COUNT) {
        queue->available_bytes = FORT_QUEUE_INITIAL_TOKEN_COUNT;
    }

    queue->last_tick = now;

    /* Move packets to the latency queue as the accumulated available bytes will allow */
    PFORT_FLOW_PACKET pkt_chain = queue->bandwidth_list.packet_head;
    if (pkt_chain == NULL)
        return;

    PFORT_FLOW_PACKET pkt_tail = NULL;
    PFORT_FLOW_PACKET pkt = pkt_chain;
    do {
        if (bps != 0LL && queue->available_bytes < pkt->data_length)
            break;

        queue->available_bytes -= pkt->data_length;
        queue->queued_bytes -= pkt->data_length;

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

    PFORT_FLOW_PACKET pkt_tail = NULL;
    PFORT_FLOW_PACKET pkt = pkt_chain;
    do {
        /* Round to the closest ms instead of truncating
         * by adding 1/2 of a ms to the elapsed ticks */
        const ULONG elapsed_ms = (ULONG) (((now.QuadPart - pkt->latency_start.QuadPart) * 1000LL
                                                  + g_QpcFrequencyHalfMs)
                / g_QpcFrequency.QuadPart);

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
    for (int i = 0; limit_io_bits != 0; ++i) {
        const BOOL queue_exists = (limit_io_bits & 1) != 0;
        limit_io_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = fort_shaper_create_queue(shaper, i);
        if (queue == NULL)
            continue;

        queue->limit = limits[i];
    }
}

static void fort_shaper_init_queues(PFORT_SHAPER shaper, UINT32 group_io_bits)
{
    if (group_io_bits == 0)
        return;

    const LARGE_INTEGER now = KeQueryPerformanceCounter(NULL);

    for (int i = 0; group_io_bits != 0; ++i) {
        const BOOL queue_exists = (group_io_bits & 1) != 0;
        group_io_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = shaper->queues[i];
        if (queue == NULL)
            continue;

        queue->queued_bytes = 0;
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

static void fort_shaper_timer_start(PFORT_SHAPER shaper)
{
    const ULONG active_io_bits = fort_shaper_io_bits(&shaper->active_io_bits);

    if (active_io_bits == 0)
        return;

    fort_timer_set_running(&shaper->timer, /*run=*/TRUE);
}

inline static ULONG fort_shaper_timer_process_queues(PFORT_SHAPER shaper, ULONG active_io_bits)
{
    ULONG new_active_io_bits = 0;

    const LARGE_INTEGER now = KeQueryPerformanceCounter(NULL);

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

static void NTAPI fort_shaper_timer_process(void)
{
    PFORT_SHAPER shaper = &fort_device()->shaper;

    ULONG active_io_bits =
            fort_shaper_io_bits_set(&shaper->active_io_bits, FORT_PACKET_FLUSH_ALL, FALSE);

    if (active_io_bits == 0)
        return;

    active_io_bits = fort_shaper_timer_process_queues(shaper, active_io_bits);

    if (active_io_bits != 0) {
        fort_shaper_io_bits_set(&shaper->active_io_bits, active_io_bits, TRUE);

        fort_shaper_timer_start(shaper);
    }
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
    const LARGE_INTEGER now = KeQueryPerformanceCounter(&g_QpcFrequency);
    g_QpcFrequencyHalfMs = g_QpcFrequency.QuadPart / 2000LL;
    g_RandomSeed = now.LowPart;

    FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_TRANSPORT, &shaper->injection_transport4_id);
    FwpsInjectionHandleCreate0(
            AF_INET6, FWPS_INJECTION_TYPE_TRANSPORT, &shaper->injection_transport6_id);

    tommy_arrayof_init(&shaper->pending.procs, sizeof(FORT_PENDING_PROC));
    tommy_arrayof_init(&shaper->pending.packets, sizeof(FORT_PENDING_PACKET));

    KeInitializeSpinLock(&shaper->pending.lock);

    tommy_arrayof_init(&shaper->packets, sizeof(FORT_FLOW_PACKET));

    KeInitializeSpinLock(&shaper->lock);

    fort_timer_open(
            &shaper->timer, /*period(ms)=*/1, FORT_TIMER_ONESHOT, &fort_shaper_timer_process);
}

FORT_API void fort_shaper_close(PFORT_SHAPER shaper)
{
    fort_timer_close(&shaper->timer);

    fort_shaper_drop_packets(shaper);
    fort_shaper_free_queues(shaper);

    tommy_arrayof_done(&shaper->packets);

    FwpsInjectionHandleDestroy0(shaper->injection_transport4_id);
    FwpsInjectionHandleDestroy0(shaper->injection_transport6_id);
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

        const UINT32 new_limit_io_bits = (flush_io_bits & limit_io_bits);

        fort_shaper_create_queues(shaper, conf_group->limits, limit_io_bits);
        fort_shaper_init_queues(shaper, new_limit_io_bits);

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

        const UINT32 new_group_io_bits = (flush_io_bits & group_io_bits);

        fort_shaper_init_queues(shaper, new_group_io_bits);

        fort_shaper_io_bits_exchange(
                &shaper->group_io_bits, (shaper->limit_io_bits & group_io_bits));
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_shaper_flush(shaper, flush_io_bits, /*drop=*/FALSE);
}

static void fort_shaper_packet_queue_add_packet(PFORT_PACKET_QUEUE queue, PFORT_FLOW_PACKET pkt)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);
    {
        queue->queued_bytes += pkt->data_length;

        fort_shaper_packet_list_add(&queue->bandwidth_list, pkt);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

inline static BOOL fort_shaper_packet_queue_check_plr(PFORT_PACKET_QUEUE queue)
{
    const UINT16 plr = queue->limit.plr;
    if (plr > 0) {
        const ULONG random = RtlRandomEx(&g_RandomSeed) % 10000; /* PLR range is 0-10000 */
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
    const ULONG data_length = fort_packet_data_length(ca->netBufList);

    /* Check the Queue for new Packet */
    if (!fort_shaper_packet_queue_check_packet(queue, data_length))
        return STATUS_SUCCESS; /* drop the packet */

    /* Create the Packet */
    PFORT_FLOW_PACKET pkt = fort_shaper_packet_get(shaper);
    if (pkt == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(pkt, sizeof(FORT_FLOW_PACKET));

    const NTSTATUS status = fort_packet_fill(ca, &pkt->io);
    if (!NT_SUCCESS(status)) {
        fort_shaper_packet_put(shaper, pkt);
        return status;
    }

    pkt->flow = flow;
    pkt->data_length = data_length;

    /* Add the Packet to Queue */
    fort_shaper_packet_queue_add_packet(queue, pkt);

    /* Packets in transport layer must be re-injected in DCP due to locking */
    fort_shaper_io_bits_set(&shaper->active_io_bits, queue_bit, TRUE);

    /* Start the Timer */
    fort_shaper_timer_start(shaper);

    return STATUS_SUCCESS;
}

inline static BOOL fort_shaper_injected_by_self(PFORT_SHAPER shaper, PCFORT_CALLOUT_ARG ca)
{
    const HANDLE injection_id = fort_shaper_injection_id(shaper, ca->isIPv6);

    return fort_packet_injected_by_self(injection_id, ca->netBufList);
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
    if (fort_shaper_injected_by_self(shaper, ca))
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

static PFORT_PENDING_PACKET fort_pending_packet_get_locked(PFORT_PENDING pending)
{
    PFORT_PENDING_PACKET pkt = NULL;

    if (pending->packet_free != NULL) {
        pkt = pending->packet_free;
        pending->packet_free = pkt->next;
    } else {
        const tommy_size_t size = tommy_arrayof_size(&pending->packets);

        /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
        if (tommy_arrayof_grow(&pending->packets, size + 1), 0)
            return NULL;

        pkt = tommy_arrayof_ref(&pending->packets, size);
    }

    return pkt;
}

static PFORT_PENDING_PACKET fort_pending_packet_get(PFORT_PENDING pending)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    PFORT_PENDING_PACKET pkt = fort_pending_packet_get_locked(pending);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pkt;
}

static void fort_pending_packet_put_locked(PFORT_PENDING pending, PFORT_PENDING_PACKET pkt)
{
    pkt->next = pending->packet_free;
    pending->packet_free = pkt;
}

static void fort_pending_packet_put(PFORT_PENDING pending, PFORT_PENDING_PACKET pkt)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    fort_pending_packet_put_locked(pending, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
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

static BOOL fort_pending_proc_check_packet_count(PFORT_PENDING pending, UINT32 process_id)
{
    UINT16 packet_count = 0;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    PFORT_PENDING_PROC proc = fort_pending_proc_find_locked(pending, process_id);
    if (proc != NULL) {
        packet_count = proc->packet_count;
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return packet_count < FORT_PENDING_PROC_PACKET_COUNT_MAX;
}

static PFORT_PENDING_PROC fort_pending_proc_get_locked(PFORT_PENDING pending, UINT32 process_id)
{
    PFORT_PENDING_PROC proc = fort_pending_proc_find_locked(pending, process_id);
    if (proc != NULL) {
        if (proc->packet_count >= FORT_PENDING_PROC_PACKET_COUNT_MAX)
            return NULL;

        return proc;
    }

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

    return proc;
}

static void fort_pending_proc_put_locked(PFORT_PENDING pending, PFORT_PENDING_PROC proc)
{
    proc->next = pending->proc_free;
    pending->proc_free = proc;
}

static NTSTATUS fort_pending_proc_add_packet_locked(PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_PENDING_PACKET pkt)
{
    /* Create the Pending Process */
    PFORT_PENDING_PROC proc = fort_pending_proc_get_locked(pending, cx->process_id);
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

FORT_API BOOL fort_packet_add_pending(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx)
{
    NTSTATUS status;

    PFORT_SHAPER shaper = &fort_device()->shaper;

    /* Skip self injected packet */
    if (ca->netBufList != NULL && fort_shaper_injected_by_self(shaper, ca))
        return FALSE;

    PFORT_PENDING pending = &shaper->pending;

    /* Check the Process's Packet Count */
    if (!fort_pending_proc_check_packet_count(pending, cx->process_id))
        return FALSE;

    /* Create the Packet */
    PFORT_PENDING_PACKET pkt = fort_pending_packet_get(pending);
    if (pkt == NULL)
        return FALSE;

    RtlZeroMemory(pkt, sizeof(FORT_PENDING_PACKET));

    status = fort_packet_fill(ca, &pkt->io);
    if (NT_SUCCESS(status)) {
        if (fort_packet_is_ipsec_protected(ca)) {
            pkt->io.flags |= FORT_PACKET_IPSEC_PROTECTED;
        }

        /* Add the Packet to Pending Process */
        status = fort_pending_proc_add_packet(pending, ca, cx, pkt);
    }

    if (!NT_SUCCESS(status)) {
        fort_pending_packet_put(pending, pkt);
        return FALSE;
    }

    return TRUE;
}
