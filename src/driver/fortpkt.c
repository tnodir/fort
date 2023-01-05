/* Fort Firewall Packets Shaping & Re-injection */

#include "fortpkt.h"

#include "fortdev.h"
#include "forttrace.h"
#include "fortutl.h"

#define FORT_PACKET_POOL_TAG 'KwfF'

#define INITIAL_TOKEN_COUNT 1500

#define fort_shaper_injection_handle(shaper, isIPv4, isIPv6)                                       \
    ((isIPv4) ? (shaper)->injection_ip4                                                            \
              : ((isIPv6) ? (shaper)->injection_ip6 : (shaper)->injection_unspec))

typedef void(NTAPI *FORT_PACKET_FOREACH_FUNC)(PFORT_SHAPER, PFORT_PACKET);

static LARGE_INTEGER g_QpcFrequency;
static UINT64 g_QpcFrequencyHalfMs;
static ULONG g_RandomSeed;

static ULONG fort_shaper_queue_active_set(PFORT_SHAPER shaper, ULONG v, BOOL on)
{
    return on ? InterlockedOr(&shaper->active_io_bits, v)
              : InterlockedAnd(&shaper->active_io_bits, ~v);
}

static ULONG fort_shaper_queue_active(PFORT_SHAPER shaper)
{
    return fort_shaper_queue_active_set(shaper, 0, TRUE);
}

static BOOL fort_packet_injected_by_self(HANDLE injection_handle, PNET_BUFFER_LIST netBufList)
{
    const FWPS_PACKET_INJECTION_STATE state =
            FwpsQueryPacketInjectionState0(injection_handle, netBufList, NULL);

    return (state == FWPS_PACKET_INJECTED_BY_SELF
            || state == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF);
}

static ULONG fort_packet_data_length(const PNET_BUFFER_LIST netBufList)
{
    ULONG data_length = 0;

    PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList);
    while (netBuf) {
        data_length += NET_BUFFER_DATA_LENGTH(netBuf);
        netBuf = NET_BUFFER_NEXT_NB(netBuf);
    }

    return data_length;
}

static NTSTATUS fort_shaper_packet_clone(PFORT_SHAPER shaper,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        PFORT_PACKET pkt)
{
    pkt->layerId = inFixedValues->layerId;

    const BOOL inbound = (pkt->flags & FORT_PACKET_INBOUND) != 0;

    const UINT if_index = inbound ? FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_INTERFACE_INDEX
                                  : FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_INTERFACE_INDEX;
    pkt->interfaceIndex = inFixedValues->incomingValue[if_index].value.uint32;

    const UINT port_index = inbound ? FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_NDIS_PORT
                                    : FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_NDIS_PORT;
    pkt->ndisPortNumber = inFixedValues->incomingValue[port_index].value.uint32;

    const UINT32 bytesRetreated = (inbound
                                          && FWPS_IS_L2_METADATA_FIELD_PRESENT(inMetaValues,
                                                  FWPS_L2_METADATA_FIELD_ETHERNET_MAC_HEADER_SIZE))
            ? inMetaValues->ethernetMacHeaderSize
            : 0;

    /* Clone the buffer */
    NTSTATUS status = STATUS_SUCCESS;

    if (bytesRetreated != 0) {
        status = NdisRetreatNetBufferDataStart(
                NET_BUFFER_LIST_FIRST_NB(netBufList), bytesRetreated, 0, 0);
    }

    if (NT_SUCCESS(status)) {
        status = FwpsAllocateCloneNetBufferList0(netBufList, NULL, NULL, 0, &pkt->netBufList);
    }

    if (bytesRetreated != 0) {
        NdisAdvanceNetBufferDataStart(
                NET_BUFFER_LIST_FIRST_NB(netBufList), bytesRetreated, FALSE, 0);
    }

    return status;
}

static void fort_shaper_packet_free(PFORT_SHAPER shaper, PFORT_PACKET pkt)
{
    UNUSED(shaper);

    if (pkt->netBufList != NULL) {
        const NTSTATUS status = pkt->netBufList->Status;

        if (!NT_SUCCESS(status) && status != STATUS_NOT_FOUND) {
            LOG("Shaper: Packet injection error: %x\n", status);
            TRACE(FORT_SHAPER_PACKET_INJECTION_ERROR, status, 0, 0);
        }

        FwpsFreeCloneNetBufferList0(pkt->netBufList, 0);
    }

    fort_mem_free(pkt, FORT_PACKET_POOL_TAG);
}

static void NTAPI fort_packet_inject_complete(
        PFORT_PACKET pkt, PNET_BUFFER_LIST clonedNetBufList, BOOLEAN dispatchLevel)
{
    UNUSED(clonedNetBufList);
    UNUSED(dispatchLevel);

    fort_shaper_packet_free(/*shaper=*/NULL, pkt);
}

static void fort_shaper_packet_inject(PFORT_SHAPER shaper, PFORT_PACKET pkt)
{
    const BOOL inbound = (pkt->flags & FORT_PACKET_INBOUND) != 0;
    const BOOL isIPv4 = (pkt->flags & FORT_PACKET_IP4) != 0;
    const BOOL isIPv6 = (pkt->flags & FORT_PACKET_IP6) != 0;
    const HANDLE injection_handle = fort_shaper_injection_handle(shaper, isIPv4, isIPv6);

    const NTSTATUS status = (inbound)
            ? FwpsInjectMacReceiveAsync0(injection_handle, NULL, 0, pkt->layerId,
                    pkt->interfaceIndex, pkt->ndisPortNumber, pkt->netBufList,
                    &fort_packet_inject_complete, pkt)
            : FwpsInjectMacSendAsync0(injection_handle, NULL, 0, pkt->layerId, pkt->interfaceIndex,
                    pkt->ndisPortNumber, pkt->netBufList, &fort_packet_inject_complete, pkt);

    if (!NT_SUCCESS(status)) {
        LOG("Shaper: Packet injection prepare error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_INJECTION_PREPARE_ERROR, status, 0, 0);

        pkt->netBufList->Status = STATUS_SUCCESS;

        fort_shaper_packet_free(/*shaper=*/NULL, pkt);
    }
}

static void fort_shaper_packet_foreach(
        PFORT_SHAPER shaper, PFORT_PACKET pkt, const FORT_PACKET_FOREACH_FUNC func)
{
    while (pkt != NULL) {
        PFORT_PACKET pkt_next = pkt->next;

        func(shaper, pkt);

        pkt = pkt_next;
    }
}

inline static BOOL fort_shaper_packet_list_is_empty(PFORT_PACKET_LIST pkt_list)
{
    return (pkt_list->packet_head == NULL);
}

static void fort_shaper_packet_list_add_chain(
        PFORT_PACKET_LIST pkt_list, PFORT_PACKET pkt_head, PFORT_PACKET pkt_tail)
{
    if (pkt_list->packet_tail == NULL) {
        pkt_list->packet_head = pkt_head;
    } else {
        pkt_list->packet_tail->next = pkt_head;
    }

    pkt_list->packet_tail = pkt_tail;
}

inline static void fort_shaper_packet_list_add(PFORT_PACKET_LIST pkt_list, PFORT_PACKET pkt)
{
    fort_shaper_packet_list_add_chain(pkt_list, pkt, pkt);
}

static PFORT_PACKET fort_shaper_packet_list_get(PFORT_PACKET_LIST pkt_list, PFORT_PACKET pkt)
{
    if (pkt_list->packet_head != NULL) {
        pkt_list->packet_tail->next = pkt;
        pkt = pkt_list->packet_head;

        pkt_list->packet_head = pkt_list->packet_tail = NULL;
    }

    return pkt;
}

static void fort_shaper_packet_list_cut_chain(PFORT_PACKET_LIST pkt_list, PFORT_PACKET pkt)
{
    pkt_list->packet_head = pkt->next;
    pkt->next = NULL;

    if (pkt_list->packet_head == NULL) {
        pkt_list->packet_tail = NULL;
    }
}

static void fort_shaper_queue_process_bandwidth(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, const LARGE_INTEGER now)
{
    const UINT64 bps = queue->limit.bps;

    /* Advance the available bytes */
    const UINT64 accumulated =
            ((now.QuadPart - queue->last_tick.QuadPart) * bps) / (8 * g_QpcFrequency.QuadPart);
    queue->available_bytes += accumulated;

    if (fort_shaper_packet_list_is_empty(&queue->bandwidth_list)
            && queue->available_bytes > INITIAL_TOKEN_COUNT) {
        queue->available_bytes = INITIAL_TOKEN_COUNT;
    }

    queue->last_tick = now;

    /* Move packets to the latency queue as the accumulated available bytes will allow */
    PFORT_PACKET pkt_chain = queue->bandwidth_list.packet_head;
    if (pkt_chain == NULL)
        return;

    PFORT_PACKET pkt_tail = NULL;
    PFORT_PACKET pkt = pkt_chain;
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

static PFORT_PACKET fort_shaper_queue_process_latency(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, const LARGE_INTEGER now)
{
    PFORT_PACKET pkt_chain = queue->latency_list.packet_head;
    if (pkt_chain == NULL)
        return NULL;

    const UINT32 latency_ms = queue->limit.latency_ms;

    PFORT_PACKET pkt_tail = NULL;
    PFORT_PACKET pkt = pkt_chain;
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

static PFORT_PACKET fort_shaper_queue_get_packets(PFORT_PACKET_QUEUE queue, PFORT_PACKET pkt)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);

    pkt = fort_shaper_packet_list_get(&queue->latency_list, pkt);
    pkt = fort_shaper_packet_list_get(&queue->bandwidth_list, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pkt;
}

inline static BOOL fort_shaper_queue_is_empty(PFORT_PACKET_QUEUE queue)
{
    return fort_shaper_packet_list_is_empty(&queue->bandwidth_list)
            || fort_shaper_packet_list_is_empty(&queue->latency_list);
}

static BOOL fort_shaper_queue_process(PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue)
{
    PFORT_PACKET pkt_chain = NULL;
    BOOL is_active = FALSE;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);

    if (!fort_shaper_queue_is_empty(queue)) {
        const LARGE_INTEGER now = KeQueryPerformanceCounter(NULL);

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

static void fort_shaper_create_queues(
        PFORT_SHAPER shaper, PFORT_SPEED_LIMIT limits, UINT32 limit_io_bits)
{
    for (int i = 0; limit_io_bits != 0; ++i) {
        const BOOL queue_exists = (limit_io_bits & 1) != 0;
        limit_io_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = shaper->queues[i];
        if (queue == NULL) {
            queue = fort_mem_alloc(sizeof(FORT_PACKET_QUEUE), FORT_PACKET_POOL_TAG);
            if (queue == NULL)
                break;

            RtlZeroMemory(queue, sizeof(FORT_PACKET_QUEUE));

            shaper->queues[i] = queue;
        }

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
        queue->available_bytes = INITIAL_TOKEN_COUNT;
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
    const ULONG active_io_bits = fort_shaper_queue_active(shaper);

    if (active_io_bits == 0)
        return;

    fort_timer_update(&shaper->timer, /*run=*/TRUE);
}

static void NTAPI fort_shaper_timer_process(void)
{
    PFORT_SHAPER shaper = &fort_device()->shaper;

    ULONG active_io_bits = fort_shaper_queue_active_set(shaper, FORT_PACKET_FLUSH_ALL, FALSE);
    ULONG new_active_io_bits = 0;

    for (int i = 0; active_io_bits != 0; ++i) {
        const BOOL queue_exists = (active_io_bits & 1) != 0;
        active_io_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = shaper->queues[i];
        if (queue == NULL)
            continue;

        if (fort_shaper_queue_process(shaper, queue)) {
            new_active_io_bits |= (1 << i);
        }
    }

    if (new_active_io_bits != 0) {
        fort_shaper_queue_active_set(shaper, new_active_io_bits, TRUE);

        fort_shaper_timer_start(shaper);
    }
}

FORT_API void fort_shaper_open(PFORT_SHAPER shaper)
{
    const LARGE_INTEGER now = KeQueryPerformanceCounter(&g_QpcFrequency);
    g_QpcFrequencyHalfMs = g_QpcFrequency.QuadPart / 2000LL;
    g_RandomSeed = now.LowPart;

    FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_L2, &shaper->injection_ip4);
    FwpsInjectionHandleCreate0(AF_INET6, FWPS_INJECTION_TYPE_L2, &shaper->injection_ip6);
    FwpsInjectionHandleCreate0(AF_UNSPEC, FWPS_INJECTION_TYPE_L2, &shaper->injection_unspec);

    fort_timer_open(&shaper->timer, /*period(ms)=*/1, /*oneshot=*/TRUE, /*coalescable=*/FALSE,
            &fort_shaper_timer_process);

    KeInitializeSpinLock(&shaper->lock);
}

FORT_API void fort_shaper_close(PFORT_SHAPER shaper)
{
    fort_timer_close(&shaper->timer);

    fort_shaper_free_queues(shaper);

    FwpsInjectionHandleDestroy0(shaper->injection_ip4);
    FwpsInjectionHandleDestroy0(shaper->injection_ip6);
    FwpsInjectionHandleDestroy0(shaper->injection_unspec);
}

FORT_API void fort_shaper_conf_update(PFORT_SHAPER shaper, const PFORT_CONF_IO conf_io)
{
    const UINT32 limit_io_bits = conf_io->conf_group.limit_io_bits;
    UINT32 flush_io_bits;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);
    {
        flush_io_bits = (limit_io_bits ^ shaper->group_io_bits);

        shaper->group_io_bits = shaper->limit_io_bits = limit_io_bits;

        const UINT32 new_limit_io_bits = (flush_io_bits & limit_io_bits);

        fort_shaper_create_queues(shaper, conf_io->conf_group.limits, new_limit_io_bits);
        fort_shaper_init_queues(shaper, new_limit_io_bits);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_shaper_flush(shaper, flush_io_bits, /*drop=*/FALSE);
}

void fort_shaper_conf_flags_update(PFORT_SHAPER shaper, const PFORT_CONF_FLAGS conf_flags)
{
    const UINT32 group_io_bits = fort_bits_duplicate16((UINT16) conf_flags->group_bits);
    UINT32 flush_io_bits;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);
    {
        flush_io_bits = (group_io_bits ^ shaper->group_io_bits);

        shaper->group_io_bits = (shaper->limit_io_bits & group_io_bits);

        const UINT32 new_group_io_bits = (flush_io_bits & group_io_bits);

        fort_shaper_init_queues(shaper, new_group_io_bits);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_shaper_flush(shaper, flush_io_bits, /*drop=*/FALSE);
}

static void fort_shaper_packet_queue_add(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, PFORT_PACKET pkt)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);
    {
        queue->queued_bytes += pkt->data_length;

        fort_shaper_packet_list_add(&queue->bandwidth_list, pkt);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static NTSTATUS fort_shaper_packet_queue(PFORT_SHAPER shaper,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        BOOL isIPv4, BOOL isIPv6, BOOL inbound, UCHAR group_index)
{
    NTSTATUS status;

    const UINT16 queue_index = group_index * 2 + (inbound ? 0 : 1);

    const UINT32 queue_bit = (1 << queue_index);
    if ((shaper->group_io_bits & queue_bit) == 0)
        return STATUS_NO_SUCH_GROUP;

    PFORT_PACKET_QUEUE queue = shaper->queues[queue_index];
    if (queue == NULL)
        return STATUS_NO_SUCH_GROUP;

    /* Check the Queue's PLR */
    const UINT16 plr = queue->limit.plr;
    if (plr > 0) {
        const ULONG random = RtlRandomEx(&g_RandomSeed) % 10000; /* PLR range is 0-10000 */
        if (random < plr)
            return STATUS_SUCCESS; /* drop the packet */
    }

    /* Calculate the Packets' Data Length */
    const ULONG data_length = fort_packet_data_length(netBufList);

    /* Check the Queue's Buffer Capacity */
    const UINT64 buffer_bytes = queue->limit.buffer_bytes;
    if (buffer_bytes > 0 && queue->queued_bytes + data_length > buffer_bytes)
        return STATUS_SUCCESS; /* drop the packet */

    /* Clone the Packet */
    PFORT_PACKET pkt = fort_mem_alloc(sizeof(FORT_PACKET), FORT_PACKET_POOL_TAG);
    if (pkt == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    pkt->next = NULL;
    pkt->data_length = data_length;
    pkt->flags = (inbound ? FORT_PACKET_INBOUND : 0) | (isIPv4 ? FORT_PACKET_IP4 : 0)
            | (isIPv6 ? FORT_PACKET_IP6 : 0);

    status = fort_shaper_packet_clone(shaper, inFixedValues, inMetaValues, netBufList, pkt);
    if (!NT_SUCCESS(status)) {
        fort_shaper_packet_free(/*shaper=*/NULL, pkt);
        return status;
    }

    /* Add the Cloned Packet to Queue */
    fort_shaper_packet_queue_add(shaper, queue, pkt);

    /* Process the Packet Queue */
    if (fort_shaper_queue_process(shaper, queue)) {
        fort_shaper_queue_active_set(shaper, queue_bit, TRUE);

        fort_shaper_timer_start(shaper);
    }

    return STATUS_SUCCESS;
}

FORT_API BOOL fort_shaper_packet_process(PFORT_SHAPER shaper,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        BOOL inbound, UCHAR group_index)
{
    const UINT ether_type_index = inbound ? FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE
                                          : FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE;
    const UINT16 ether_type = inFixedValues->incomingValue[ether_type_index].value.uint16;

    const BOOL isIPv4 = (ether_type == 0x8600);
    const BOOL isIPv6 = (ether_type == 0x86DD);
    const HANDLE injection_handle = fort_shaper_injection_handle(shaper, isIPv4, isIPv6);

    /* Skip self injected packet */
    if (fort_packet_injected_by_self(injection_handle, netBufList))
        return FALSE;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);

    const NTSTATUS status = fort_shaper_packet_queue(&fort_device()->shaper, inFixedValues,
            inMetaValues, netBufList, inbound, isIPv4, isIPv6, group_index);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return NT_SUCCESS(status);
}

FORT_API void fort_shaper_flush(PFORT_SHAPER shaper, UINT32 group_io_bits, BOOL drop)
{
    if (group_io_bits == 0)
        return;

    /* Collect packets from Queues */
    PFORT_PACKET pkt_chain = NULL;

    group_io_bits &= fort_shaper_queue_active_set(shaper, group_io_bits, FALSE);

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

    /* Process the packets */
    if (pkt_chain != NULL) {
        fort_shaper_packet_foreach(
                shaper, pkt_chain, (drop ? &fort_shaper_packet_free : &fort_shaper_packet_inject));
    }
}
