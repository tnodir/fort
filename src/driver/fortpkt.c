/* Fort Firewall ACK Packets Deferring & Re-injection */

#include "fortpkt.h"

#define HTONL(l) _byteswap_ulong(l)
#define NTOHL(l) HTONL(l)
#define HTONS(s) _byteswap_ushort(s)
#define NTOHS(s) HTONS(s)

static NTSTATUS fort_packet_inject_in(PFORT_DEFER defer, PFORT_PACKET pkt,
        PNET_BUFFER_LIST *clonedNetBufList, FORT_INJECT_COMPLETE_FUNC complete_func)
{
    PFORT_PACKET_IN pkt_in = &pkt->in;
    PNET_BUFFER_LIST netBufList = pkt->netBufList;
    PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(netBufList);
    NTSTATUS status;

    /* The TCP/IP stack could have retreated the net buffer list by the
     * transportHeaderSize amount; detect the condition here to avoid
     * retreating twice.
     */
    if (pkt->dataOffset != NET_BUFFER_DATA_OFFSET(netBuffer)) {
        pkt_in->transportHeaderSize = 0;
    }

    /* Adjust the net buffer list offset to the start of the IP header. */
    NdisRetreatNetBufferDataStart(
            netBuffer, pkt_in->ipHeaderSize + pkt_in->transportHeaderSize, 0, NULL);

    status = FwpsAllocateCloneNetBufferList0(netBufList, NULL, NULL, 0, clonedNetBufList);

    /* Undo the adjustment on the original net buffer list. */
    NdisAdvanceNetBufferDataStart(
            netBuffer, pkt_in->ipHeaderSize + pkt_in->transportHeaderSize, FALSE, NULL);

    if (NT_SUCCESS(status)) {
        status = FwpsInjectTransportReceiveAsync0(defer->transport_injection4_id, NULL, NULL, 0,
                AF_INET, pkt_in->compartmentId, pkt_in->interfaceIndex, pkt_in->subInterfaceIndex,
                *clonedNetBufList, (FWPS_INJECT_COMPLETE0) complete_func, pkt);
    }

    return status;
}

static NTSTATUS fort_packet_inject_out(PFORT_DEFER defer, PFORT_PACKET pkt,
        PNET_BUFFER_LIST *clonedNetBufList, FORT_INJECT_COMPLETE_FUNC complete_func)
{
    PFORT_PACKET_OUT pkt_out = &pkt->out;
    PNET_BUFFER_LIST netBufList = pkt->netBufList;
    NTSTATUS status;

    status = FwpsAllocateCloneNetBufferList0(netBufList, NULL, NULL, 0, clonedNetBufList);

    if (NT_SUCCESS(status)) {
        FWPS_TRANSPORT_SEND_PARAMS0 send_args;

        RtlZeroMemory(&send_args, sizeof(FWPS_TRANSPORT_SEND_PARAMS0));
        send_args.remoteAddress = (UCHAR *) &pkt_out->remoteAddr4;
        send_args.remoteScopeId = pkt_out->remoteScopeId;

        status = FwpsInjectTransportSendAsync0(defer->transport_injection4_id, NULL,
                pkt_out->endpointHandle, 0, &send_args, AF_INET, pkt_out->compartmentId,
                *clonedNetBufList, (FWPS_INJECT_COMPLETE0) complete_func, pkt);
    }

    return status;
}

static NTSTATUS fort_packet_inject_stream(PFORT_DEFER defer, PFORT_PACKET pkt,
        PNET_BUFFER_LIST *clonedNetBufList, FORT_INJECT_COMPLETE_FUNC complete_func)
{
    PFORT_PACKET_STREAM pkt_stream = &pkt->stream;
    PNET_BUFFER_LIST netBufList = pkt->netBufList;
    PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(netBufList);
    NTSTATUS status;

    /* Adjust the net buffer list offset to the start of the actual data. */
    NdisAdvanceNetBufferDataStart(netBuffer, pkt->dataOffset, FALSE, NULL);

    status = FwpsAllocateCloneNetBufferList0(netBufList, NULL, NULL, 0, clonedNetBufList);

    /* Undo the adjustment on the original net buffer list. */
    NdisRetreatNetBufferDataStart(netBuffer, pkt->dataOffset, 0, NULL);

    if (NT_SUCCESS(status)) {
        status = FwpsStreamInjectAsync0(defer->stream_injection4_id, NULL, 0, pkt_stream->flow_id,
                pkt_stream->calloutId, pkt_stream->layerId, pkt_stream->streamFlags,
                *clonedNetBufList, pkt->dataSize, (FWPS_INJECT_COMPLETE0) complete_func, pkt);
    }

    return status;
}

static BOOL fort_packet_injected_by_self(PFORT_DEFER defer, PNET_BUFFER_LIST netBufList)
{
    const FWPS_PACKET_INJECTION_STATE state =
            FwpsQueryPacketInjectionState0(defer->transport_injection4_id, netBufList, NULL);

    return (state == FWPS_PACKET_INJECTED_BY_SELF
            || state == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF);
}

static PFORT_PACKET fort_defer_packet_get(PFORT_DEFER defer)
{
    PFORT_PACKET pkt = NULL;

    if (defer->packet_free != NULL) {
        pkt = defer->packet_free;
        defer->packet_free = pkt->next;
    } else {
        const tommy_size_t size = tommy_arrayof_size(&defer->packets);

        /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
        if (tommy_arrayof_grow(&defer->packets, size + 1), 0)
            return NULL;

        pkt = tommy_arrayof_ref(&defer->packets, size);
    }

    return pkt;
}

static void fort_defer_packet_put(PFORT_DEFER defer, PFORT_PACKET pkt)
{
    pkt->next = defer->packet_free;
    defer->packet_free = pkt;
}

FORT_API void fort_defer_open(PFORT_DEFER defer)
{
    NTSTATUS status;

    status = FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_TRANSPORT, &defer->transport_injection4_id);

    if (!NT_SUCCESS(status)) {
        defer->transport_injection4_id = INVALID_HANDLE_VALUE;

        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                "FORT: Defer: Transport injection init error: %x\n", status);
    }

    status = FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_STREAM, &defer->stream_injection4_id);

    if (!NT_SUCCESS(status)) {
        defer->stream_injection4_id = INVALID_HANDLE_VALUE;

        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                "FORT: Defer: Stream injection init error: %x\n", status);
    }

    tommy_arrayof_init(&defer->packets, sizeof(FORT_PACKET));

    KeInitializeSpinLock(&defer->lock);
}

FORT_API void fort_defer_close(PFORT_DEFER defer)
{
    FwpsInjectionHandleDestroy0(defer->transport_injection4_id);
    FwpsInjectionHandleDestroy0(defer->stream_injection4_id);

    tommy_arrayof_done(&defer->packets);
}

static void fort_defer_list_add(PFORT_DEFER_LIST defer_list, PFORT_PACKET pkt)
{
    if (defer_list->packet_tail == NULL) {
        defer_list->packet_head = defer_list->packet_tail = pkt;
    } else {
        defer_list->packet_tail->next = pkt;
        defer_list->packet_tail = pkt;
    }
}

static PFORT_PACKET fort_defer_list_get(PFORT_DEFER_LIST defer_list, PFORT_PACKET pkt_chain)
{
    if (defer_list->packet_head != NULL) {
        defer_list->packet_tail->next = pkt_chain;
        pkt_chain = defer_list->packet_head;

        defer_list->packet_head = defer_list->packet_tail = NULL;
    }

    return pkt_chain;
}

static PFORT_PACKET fort_defer_list_flow_get(
        PFORT_DEFER_LIST defer_list, PFORT_PACKET pkt_chain, UINT64 flow_id)
{
    if (flow_id == FORT_DEFER_STREAM_ALL) {
        return fort_defer_list_get(defer_list, pkt_chain);
    }

    if (defer_list->packet_head != NULL) {
        PFORT_PACKET pkt_tail = pkt_chain;
        PFORT_PACKET pkt_prev = NULL;
        PFORT_PACKET pkt = defer_list->packet_head;

        do {
            PFORT_PACKET pkt_next = pkt->next;

            if (flow_id == pkt->stream.flow_id) {
                if (pkt_prev == NULL) {
                    pkt_chain = pkt;
                } else {
                    pkt_prev->next = pkt;
                }

                pkt_prev = pkt;
                pkt->next = pkt_tail;

                if (pkt == defer_list->packet_head) {
                    defer_list->packet_head = pkt_next;
                }

                if (pkt == defer_list->packet_tail) {
                    defer_list->packet_tail = NULL;
                    break;
                }
            }

            pkt = pkt_next;
        } while (pkt != NULL);
    }

    return pkt_chain;
}

FORT_API NTSTATUS fort_defer_packet_add(PFORT_DEFER defer,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        BOOL inbound, UCHAR group_index)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    NTSTATUS status;

    if (defer->transport_injection4_id == INVALID_HANDLE_VALUE)
        return STATUS_FWP_TCPIP_NOT_READY;

    /* Skip self injected packet */
    if (fort_packet_injected_by_self(defer, netBufList))
        return STATUS_CANT_TERMINATE_SELF;

    /* Skip IpSec protected packet */
    if (inbound) {
        FWPS_PACKET_LIST_INFORMATION info;

        status = FwpsGetPacketListSecurityInformation0(netBufList,
                FWPS_PACKET_LIST_INFORMATION_QUERY_IPSEC
                        | FWPS_PACKET_LIST_INFORMATION_QUERY_INBOUND,
                &info);

        if (!NT_SUCCESS(status))
            return status;

        if (info.ipsecInformation.inbound.isSecure)
            return STATUS_UNSUCCESSFUL;
    }

    const UINT16 list_index = group_index * 2 + (inbound ? 0 : 1);

    KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);

    PFORT_DEFER_LIST defer_list = &defer->lists[list_index];

    PFORT_PACKET pkt = fort_defer_packet_get(defer);
    if (pkt == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    fort_defer_list_add(defer_list, pkt);

    pkt->inbound = inbound;
    pkt->is_stream = FALSE;
    pkt->dataOffset = 0;
    pkt->dataSize = 0; /* not used */
    pkt->netBufList = netBufList;
    pkt->next = NULL;

    if (inbound) {
        PFORT_PACKET_IN pkt_in = &pkt->in;

        pkt_in->compartmentId = inMetaValues->compartmentId;

        const int interfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_INDEX;
        const int subInterfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX;

        pkt_in->interfaceIndex = inFixedValues->incomingValue[interfaceField].value.uint32;
        pkt_in->subInterfaceIndex = inFixedValues->incomingValue[subInterfaceField].value.uint32;

        pkt_in->ipHeaderSize = (UINT16) inMetaValues->ipHeaderSize;
        pkt_in->transportHeaderSize = (UINT16) inMetaValues->transportHeaderSize;

        pkt->dataOffset = (UINT16) NET_BUFFER_DATA_OFFSET(NET_BUFFER_LIST_FIRST_NB(netBufList));
    } else {
        PFORT_PACKET_OUT pkt_out = &pkt->out;

        pkt_out->compartmentId = inMetaValues->compartmentId;

        const int remoteAddrField = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS;

        /* host-order -> network-order conversion */
        pkt_out->remoteAddr4 = HTONL(inFixedValues->incomingValue[remoteAddrField].value.uint32);

        pkt_out->remoteScopeId = inMetaValues->remoteScopeId;
        pkt_out->endpointHandle = inMetaValues->transportEndpointHandle;
    }

    FwpsReferenceNetBufferList0(pkt->netBufList, TRUE);

    /* Set to be flushed bit */
    defer->list_bits |= (1 << list_index);

    status = STATUS_SUCCESS;

end:
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return status;
}

FORT_API NTSTATUS fort_defer_stream_add(PFORT_DEFER defer,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const FWPS_STREAM_DATA0 *streamData,
        const FWPS_FILTER0 *filter, BOOL inbound)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    NTSTATUS status;

    if (defer->stream_injection4_id == INVALID_HANDLE_VALUE)
        return STATUS_FWP_TCPIP_NOT_READY;

    /* Check data offset compatibility */
    {
        const PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(streamData->netBufferListChain);
        const FWPS_STREAM_DATA_OFFSET0 *dataOffset = &streamData->dataOffset;

        if (dataOffset->streamDataOffset != 0
                && (dataOffset->netBuffer != netBuf
                        || dataOffset->mdl != NET_BUFFER_CURRENT_MDL(netBuf)))
            return STATUS_FWP_CANNOT_PEND;
    }

    KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);

    PFORT_PACKET pkt = fort_defer_packet_get(defer);
    if (pkt == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    fort_defer_list_add(&defer->stream_list, pkt);

    pkt->inbound = inbound;
    pkt->is_stream = TRUE;
    pkt->dataOffset = (UINT32) streamData->dataOffset.streamDataOffset;
    pkt->dataSize = (UINT32) streamData->dataLength;
    pkt->netBufList = streamData->netBufferListChain;
    pkt->next = NULL;

    {
        PFORT_PACKET_STREAM pkt_stream = &pkt->stream;

        pkt_stream->layerId = inFixedValues->layerId;
        pkt_stream->streamFlags = streamData->flags;
        pkt_stream->calloutId = filter->action.calloutId;
        pkt_stream->flow_id = inMetaValues->flowHandle;
    }

    FwpsReferenceNetBufferList0(pkt->netBufList, TRUE);

    status = STATUS_SUCCESS;

end:
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return status;
}

FORT_API void fort_defer_packet_free(
        PFORT_DEFER defer, PFORT_PACKET pkt, PNET_BUFFER_LIST clonedNetBufList, BOOL dispatchLevel)
{
    KLOCK_QUEUE_HANDLE lock_queue;

    if (clonedNetBufList != NULL) {
        const NTSTATUS status = clonedNetBufList->Status;

        if (!NT_SUCCESS(status)) {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: Defer: Injection error: %x\n", status);
        }

        FwpsFreeCloneNetBufferList0(clonedNetBufList, 0);
    }

    FwpsDereferenceNetBufferList0(pkt->netBufList, FALSE);

    /* Add to free chain */
    if (dispatchLevel) {
        KeAcquireInStackQueuedSpinLockAtDpcLevel(&defer->lock, &lock_queue);
    } else {
        KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);
    }

    fort_defer_packet_put(defer, pkt);

    if (dispatchLevel) {
        KeReleaseInStackQueuedSpinLockFromDpcLevel(&lock_queue);
    } else {
        KeReleaseInStackQueuedSpinLock(&lock_queue);
    }
}

static void fort_defer_packet_inject(PFORT_DEFER defer, PFORT_PACKET pkt,
        FORT_INJECT_COMPLETE_FUNC complete_func, BOOL dispatchLevel)
{
    while (pkt != NULL) {
        PFORT_PACKET pkt_next = pkt->next;
        PNET_BUFFER_LIST clonedNetBufList = NULL;
        NTSTATUS status;

        const FORT_INJECT_FUNC inject_func = pkt->is_stream
                ? &fort_packet_inject_stream
                : (pkt->inbound ? &fort_packet_inject_in : &fort_packet_inject_out);

        status = inject_func(defer, pkt, &clonedNetBufList, complete_func);

        if (!NT_SUCCESS(status)) {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: Defer: Injection prepare error: %x\n", status);

            if (clonedNetBufList != NULL) {
                clonedNetBufList->Status = STATUS_SUCCESS;
            }

            fort_defer_packet_free(defer, pkt, clonedNetBufList, dispatchLevel);
        }

        pkt = pkt_next;
    }
}

FORT_API void fort_defer_packet_flush(PFORT_DEFER defer, FORT_INJECT_COMPLETE_FUNC complete_func,
        UINT32 list_bits, BOOL dispatchLevel)
{
    KLOCK_QUEUE_HANDLE lock_queue;

    list_bits &= defer->list_bits;
    if (list_bits == 0)
        return;

    PFORT_PACKET pkt_chain = NULL;

    if (dispatchLevel) {
        KeAcquireInStackQueuedSpinLockAtDpcLevel(&defer->lock, &lock_queue);
    } else {
        KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);
    }

    for (int i = 0; list_bits != 0; ++i) {
        const UINT32 list_bit = (list_bits & 1);

        list_bits >>= 1;
        if (list_bit == 0)
            continue;

        PFORT_DEFER_LIST defer_list = &defer->lists[i];

        pkt_chain = fort_defer_list_get(defer_list, pkt_chain);
    }

    /* Clear flushed bits */
    defer->list_bits &= ~list_bits;

    if (dispatchLevel) {
        KeReleaseInStackQueuedSpinLockFromDpcLevel(&lock_queue);
    } else {
        KeReleaseInStackQueuedSpinLock(&lock_queue);
    }

    fort_defer_packet_inject(defer, pkt_chain, complete_func, dispatchLevel);
}

FORT_API void fort_defer_stream_flush(PFORT_DEFER defer, FORT_INJECT_COMPLETE_FUNC complete_func,
        UINT64 flow_id, BOOL dispatchLevel)
{
    KLOCK_QUEUE_HANDLE lock_queue;

    if (dispatchLevel) {
        KeAcquireInStackQueuedSpinLockAtDpcLevel(&defer->lock, &lock_queue);
    } else {
        KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);
    }

    PFORT_PACKET pkt_chain = fort_defer_list_flow_get(&defer->stream_list, NULL, flow_id);

    if (dispatchLevel) {
        KeReleaseInStackQueuedSpinLockFromDpcLevel(&lock_queue);
    } else {
        KeReleaseInStackQueuedSpinLock(&lock_queue);
    }

    fort_defer_packet_inject(defer, pkt_chain, complete_func, dispatchLevel);
}
