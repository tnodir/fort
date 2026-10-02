/* Fort Firewall Packets Shaping */

#include "fortpkt_shaper.h"

#include "fortdev.h"
#include "fortutl.h"

#define FORT_PACKET_FLUSH_ALL 0xFFFFFFFF

#define FORT_QUEUE_INITIAL_TOKEN_COUNT 1500

#define FORT_QUEUE_ELAPSED_SECONDS_MAX 3600 /* to repay the big packet's debt */

/* The held inbound packets' clones keep the NIC's receive buffers, so limit their count */
#define FORT_SHAPER_IN_PACKET_COUNT_MAX 512

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

inline static UINT32 fort_shaper_limit_bit(UCHAR limit_id)
{
    if (limit_id == 0 || limit_id > FORT_CONF_SPEED_LIMIT_MAX)
        return 0;

    return (1u << (limit_id - 1));
}

inline static PFORT_FLOW_PACKET fort_shaper_packet_new(void)
{
    return fort_mem_alloc(sizeof(FORT_FLOW_PACKET), FORT_PACKET_POOL_TAG);
}

inline static void fort_shaper_packet_del(PFORT_FLOW_PACKET pkt)
{
    fort_mem_free(pkt, FORT_PACKET_POOL_TAG);
}

FORT_API void fort_shaper_packet_free(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    if ((pkt->io.flags & FORT_PACKET_INBOUND) != 0) {
        InterlockedDecrement(&shaper->in_packet_count);
    }

    fort_packet_free(&pkt->io);

    fort_shaper_packet_del(pkt);
}

static void fort_shaper_packet_drop(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    fort_shaper_packet_free(shaper, pkt);
}

static void fort_shaper_packet_inject(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt)
{
    NTSTATUS status;

    status = fort_packet_inject(&pkt->io);

    if (!NT_SUCCESS(status)) {
        fort_shaper_packet_free(shaper, pkt);
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

static PFORT_FLOW_PACKET fort_shaper_packet_list_get_flow_packets(PFORT_PACKET_LIST pkt_list,
        PFORT_FLOW flow, PFORT_FLOW_PACKET pkt_chain, UINT64 *data_length)
{
    PFORT_FLOW_PACKET pkt_prev = NULL;
    PFORT_FLOW_PACKET pkt = pkt_list->packet_head;

    while (pkt != NULL) {
        PFORT_FLOW_PACKET pkt_next = pkt->next;

        if (pkt->flow == flow) {
            fort_shaper_packet_list_cut_packet(pkt_list, pkt, pkt_prev, pkt_next);

            *data_length += pkt->data_length;

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
    const INT64 qpcFrequency = shaper->qpcFrequency.QuadPart;

    INT64 elapsed_ticks = now.QuadPart - last_tick.QuadPart;
    if (elapsed_ticks < 0) {
        elapsed_ticks = 0;
    }

    /* The debt is repaid by the elapsed seconds, so limit them
     * to avoid the multiplication's overflow after a long idle */
    UINT64 elapsed_seconds = (UINT64) (elapsed_ticks / qpcFrequency);
    if (elapsed_seconds > FORT_QUEUE_ELAPSED_SECONDS_MAX) {
        elapsed_seconds = FORT_QUEUE_ELAPSED_SECONDS_MAX;
    }

    const UINT64 elapsed_rem_ticks = (UINT64) (elapsed_ticks % qpcFrequency);

    /* Advance the available bytes, keep the fractional remainder for the next time */
    const UINT64 accumulated_ticks = bps * elapsed_rem_ticks + queue->available_rem;
    const UINT64 accumulated = bps * elapsed_seconds + accumulated_ticks / (UINT64) qpcFrequency;

    queue->available_rem = accumulated_ticks % (UINT64) qpcFrequency;

    queue->available_bytes += (INT64) accumulated;

    const INT64 max_available = (INT64) bps;
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
    UNUSED(shaper);

    /* Move packets to the latency queue as the accumulated available bytes will allow */
    PFORT_FLOW_PACKET pkt_chain = queue->bandwidth_list.packet_head;
    if (pkt_chain == NULL)
        return;

    const INT64 max_available = (INT64) queue->limit.bps;

    PFORT_FLOW_PACKET pkt_tail = NULL;
    PFORT_FLOW_PACKET pkt = pkt_chain;
    do {
        const INT64 pkt_length = pkt->data_length;

        /* The packet bigger than 1 second's bandwidth is sent in debt, when the bytes are full */
        if (queue->available_bytes < pkt_length && queue->available_bytes < max_available)
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

    /* Keep the packets' order: the latency list's older packets are first */
    pkt = fort_shaper_packet_list_get(&queue->bandwidth_list, pkt);
    pkt = fort_shaper_packet_list_get(&queue->latency_list, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pkt;
}

static PFORT_FLOW_PACKET fort_shaper_queue_get_flow_packets(
        PFORT_PACKET_QUEUE queue, PFORT_FLOW flow, PFORT_FLOW_PACKET pkt)
{
    UINT64 bandwidth_length = 0;
    UINT64 latency_length = 0;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);

    pkt = fort_shaper_packet_list_get_flow_packets(
            &queue->bandwidth_list, flow, pkt, &bandwidth_length);
    pkt = fort_shaper_packet_list_get_flow_packets(
            &queue->latency_list, flow, pkt, &latency_length);

    /* The queued bytes are counted for the bandwidth list only */
    queue->queued_bytes -= bandwidth_length;

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pkt;
}

inline static BOOL fort_shaper_queue_is_empty(PFORT_PACKET_QUEUE queue)
{
    return fort_shaper_packet_list_is_empty(&queue->bandwidth_list)
            && fort_shaper_packet_list_is_empty(&queue->latency_list);
}

static BOOL fort_shaper_queue_process(PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue)
{
    PFORT_FLOW_PACKET pkt_chain = NULL;
    BOOL is_active = FALSE;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);

    if (!fort_shaper_queue_is_empty(queue)) {
        const LARGE_INTEGER now = KeQueryPerformanceCounter(NULL);

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

inline static PFORT_PACKET_QUEUE fort_shaper_create_queue(
        PFORT_SHAPER shaper, int queue_index, const LARGE_INTEGER now)
{
    PFORT_PACKET_QUEUE queue = shaper->queues[queue_index];
    if (queue != NULL)
        return queue;

    queue = fort_mem_alloc(sizeof(FORT_PACKET_QUEUE), FORT_PACKET_POOL_TAG);
    if (queue == NULL)
        return NULL;

    RtlZeroMemory(queue, sizeof(FORT_PACKET_QUEUE));

    /* The existing queue keeps its available bytes on the conf update */
    queue->available_bytes = FORT_QUEUE_INITIAL_TOKEN_COUNT;
    queue->last_tick = now;

    KeInitializeSpinLock(&queue->lock);

    shaper->queues[queue_index] = queue;

    return queue;
}

static void fort_shaper_create_queues(
        PFORT_SHAPER shaper, PCFORT_SPEED_LIMIT limits, UINT32 limit_bits)
{
    const LARGE_INTEGER now = KeQueryPerformanceCounter(NULL);

    for (int i = 0; limit_bits != 0; ++i) {
        const BOOL queue_exists = (limit_bits & 1) != 0;
        limit_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = fort_shaper_create_queue(shaper, i, now);
        if (queue == NULL)
            continue;

        /* The shaper's thread uses it under the queue's lock */
        KLOCK_QUEUE_HANDLE lock_queue;
        KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);
        {
            queue->limit = limits[i];
        }
        KeReleaseInStackQueuedSpinLock(&lock_queue);
    }
}

static void fort_shaper_free_queues(PFORT_SHAPER shaper)
{
    for (int i = 0; i < FORT_CONF_SPEED_LIMIT_MAX; ++i) {
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

inline static ULONG fort_shaper_thread_process_queues(PFORT_SHAPER shaper, ULONG active_io_bits)
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

        if (fort_shaper_queue_process(shaper, queue)) {
            new_active_io_bits |= (1u << i);
        }
    }

    return new_active_io_bits;
}

inline static BOOL fort_shaper_thread_process(PFORT_SHAPER shaper)
{
    ULONG active_io_bits =
            fort_shaper_io_bits_set(&shaper->active_io_bits, FORT_PACKET_FLUSH_ALL, FALSE);

    if (active_io_bits == 0)
        return FALSE;

    active_io_bits = fort_shaper_thread_process_queues(shaper, active_io_bits);

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

        const BOOL is_active = fort_shaper_thread_process(shaper);

        timeout = is_active ? &delay : NULL;

    } while ((fort_shaper_flags(shaper) & FORT_SHAPER_CLOSED) == 0);
}

static void fort_shaper_thread_close(PFORT_SHAPER shaper)
{
    fort_shaper_thread_set_event(shaper);

    fort_thread_wait(&shaper->thread);
}

inline static PFORT_FLOW_PACKET fort_shaper_flush_queues(PFORT_SHAPER shaper, UINT32 queue_bits)
{
    PFORT_FLOW_PACKET pkt_chain = NULL;

    for (int i = 0; queue_bits != 0; ++i) {
        const BOOL queue_exists = (queue_bits & 1) != 0;
        queue_bits >>= 1;

        if (!queue_exists)
            continue;

        PFORT_PACKET_QUEUE queue = shaper->queues[i];
        if (queue == NULL)
            continue;

        pkt_chain = fort_shaper_queue_get_packets(queue, pkt_chain);
    }

    return pkt_chain;
}

static void fort_shaper_flush(PFORT_SHAPER shaper, UINT32 queue_bits, BOOL drop)
{
    if (queue_bits == 0)
        return;

    /* The active bits are cleared while the thread processes the queues, so flush all given */
    fort_shaper_io_bits_set(&shaper->active_io_bits, queue_bits, FALSE);

    /* Collect packets from Queues */
    PFORT_FLOW_PACKET pkt_chain = fort_shaper_flush_queues(shaper, queue_bits);

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

/* Returns the bits of the queues to flush */
inline static UINT32 fort_shaper_enabled_bits_update_locked(PFORT_SHAPER shaper)
{
    const BOOL shaper_enabled = (fort_shaper_flags(shaper) & FORT_SHAPER_ENABLED) != 0;

    const UINT32 enabled_bits =
            shaper_enabled ? (shaper->limit_bits & shaper->limit_enabled_bits) : 0;

    /* Flush the queues, which are shaped no more (e.g. the Speed Limit is disabled) */
    const UINT32 flush_bits = (enabled_bits ^ shaper->enabled_bits);

    fort_shaper_io_bits_exchange(&shaper->enabled_bits, enabled_bits);

    return flush_bits;
}

FORT_API void fort_shaper_speed_limits_set(
        PFORT_SHAPER shaper, PCFORT_CONF_SPEED_LIMITS speed_limits)
{
    const UINT32 limit_bits = (speed_limits != NULL) ? speed_limits->mask : 0;
    UINT32 flush_bits;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);
    {
        if (speed_limits != NULL) {
            fort_shaper_create_queues(shaper, speed_limits->limits, limit_bits);
        }

        shaper->limit_bits = limit_bits;
        shaper->limit_enabled_bits = (speed_limits != NULL) ? speed_limits->enabled_mask : 0;

        flush_bits = fort_shaper_enabled_bits_update_locked(shaper);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_shaper_flush(shaper, flush_bits, /*drop=*/FALSE);
}

FORT_API void fort_shaper_speed_limit_flags_set(
        PFORT_SHAPER shaper, PCFORT_CONF_SPEED_LIMIT_FLAGS limit_flags)
{
    UINT32 flush_bits;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);
    {
        shaper->limit_enabled_bits = limit_flags->enabled_mask;

        flush_bits = fort_shaper_enabled_bits_update_locked(shaper);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_shaper_flush(shaper, flush_bits, /*drop=*/FALSE);
}

FORT_API void fort_shaper_conf_flags_update(PFORT_SHAPER shaper, const FORT_CONF_FLAGS conf_flags)
{
    UINT32 flush_bits;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&shaper->lock, &lock_queue);
    {
        fort_shaper_flags_set(shaper, FORT_SHAPER_ENABLED,
                conf_flags.filter_enabled && conf_flags.speed_limiter_enabled);

        flush_bits = fort_shaper_enabled_bits_update_locked(shaper);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_shaper_flush(shaper, flush_bits, /*drop=*/FALSE);
}

static void fort_shaper_packet_queue_add_packet(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, PFORT_FLOW_PACKET pkt, UINT32 queue_bit)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);
    {
        /* Limit the idle queue's available bytes by its first packet */
        if (fort_shaper_packet_list_is_empty(&queue->bandwidth_list)) {
            fort_shaper_queue_advance_available(shaper, queue, KeQueryPerformanceCounter(NULL));

            if (queue->available_bytes > FORT_QUEUE_INITIAL_TOKEN_COUNT) {
                queue->available_bytes = FORT_QUEUE_INITIAL_TOKEN_COUNT;
            }
        }

        queue->queued_bytes += pkt->data_length;

        fort_shaper_packet_list_add_chain(&queue->bandwidth_list, pkt, pkt);

        fort_shaper_io_bits_set(&shaper->active_io_bits, queue_bit, TRUE);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

/* RtlRandomEx() can't be called at DISPATCH_LEVEL, so use the LCG */
inline static UINT32 fort_shaper_random(PFORT_SHAPER shaper)
{
    const UINT32 seed = shaper->randomSeed * 1664525u + 1013904223u;

    shaper->randomSeed = seed;

    return seed >> 8; /* the low bits are less random */
}

inline static BOOL fort_shaper_packet_queue_check_plr(PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue)
{
    const UINT16 plr = queue->limit.plr;
    if (plr > 0) {
        const UINT32 random = fort_shaper_random(shaper) % 10000; /* PLR range is 0-10000 */
        if (random < plr)
            return FALSE;
    }
    return TRUE;
}

inline static BOOL fort_shaper_packet_queue_check_buffer(
        PFORT_PACKET_QUEUE queue, ULONG data_length)
{
    const UINT32 buffer_bytes = queue->limit.buffer_bytes;
    if (buffer_bytes == 0)
        return TRUE;

    const UINT64 queued_bytes = queue->queued_bytes;

    /* Accept a packet bigger than the buffer into the empty queue */
    return queued_bytes == 0 || (UINT64) buffer_bytes >= (queued_bytes + data_length);
}

static BOOL fort_shaper_packet_queue_check_packet(
        PFORT_SHAPER shaper, PFORT_PACKET_QUEUE queue, ULONG data_length)
{
    BOOL res;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&queue->lock, &lock_queue);
    {
        res = fort_shaper_packet_queue_check_plr(shaper, queue)
                && fort_shaper_packet_queue_check_buffer(queue, data_length);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return res;
}

inline static BOOL fort_shaper_packet_queue_check_in_count(PFORT_SHAPER shaper, BOOL inbound)
{
    return !inbound || shaper->in_packet_count < FORT_SHAPER_IN_PACKET_COUNT_MAX;
}

inline static NTSTATUS fort_shaper_packet_queue(
        PFORT_SHAPER shaper, PCFORT_CALLOUT_ARG ca, PFORT_FLOW flow)
{
    const FORT_SPEED_LIMIT_IDS speed_limits = flow->speed_limits;
    const UCHAR limit_id = ca->inbound ? speed_limits.in_limit_id : speed_limits.out_limit_id;

    const UINT32 enabled_bits = fort_shaper_io_bits(&shaper->enabled_bits);

    const UINT32 queue_bit = fort_shaper_limit_bit(limit_id);
    if ((enabled_bits & queue_bit) == 0)
        return STATUS_NO_SUCH_GROUP;

    PFORT_PACKET_QUEUE queue = shaper->queues[limit_id - 1];
    if (queue == NULL)
        return STATUS_NO_SUCH_GROUP;

    /* Check the Queue for new Packet */
    if (!fort_shaper_packet_queue_check_in_count(shaper, ca->inbound))
        return STATUS_SUCCESS; /* drop the packet */

    if (!fort_shaper_packet_queue_check_packet(shaper, queue, ca->dataSize))
        return STATUS_SUCCESS; /* drop the packet */

    /* Create the Packet */
    PFORT_FLOW_PACKET pkt = fort_shaper_packet_new();
    if (pkt == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(pkt, sizeof(FORT_FLOW_PACKET));

    const NTSTATUS status = fort_packet_fill(ca, &pkt->io, FORT_PACKET_TYPE_FLOW);

    /* The filled inbound packet is counted until its free */
    if ((pkt->io.flags & FORT_PACKET_INBOUND) != 0) {
        InterlockedIncrement(&shaper->in_packet_count);
    }

    if (!NT_SUCCESS(status)) {
        fort_shaper_packet_free(shaper, pkt);
        return status;
    }

    pkt->flow = flow;
    pkt->data_length = ca->dataSize;

    /* Add the Packet to Queue */
    fort_shaper_packet_queue_add_packet(shaper, queue, pkt, queue_bit);

    /* Packets in transport layer must be re-injected in DCP/thread due to locking */
    fort_shaper_thread_set_event(shaper);

    return STATUS_SUCCESS;
}

FORT_API BOOL fort_shaper_packet_process(PFORT_SHAPER shaper, PFORT_CALLOUT_ARG ca)
{
    if (ca->netBufList == NULL)
        return FALSE;

    if (FWPS_IS_METADATA_FIELD_PRESENT(
                ca->inMetaValues, FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED)) {
        /* Skip the packet that needs to be re-classified */
        return FALSE;
    }

    if (ca->inbound && fort_packet_is_ipsec_protected(ca)) {
        /* To be compatible with Vista's IpSec implementation, we must not
         * intercept not-yet-detunneled IpSec traffic.
         * Also the IpSec protected packets are re-injected with the rebuilt IP header only. */
        return FALSE;
    }

    if (fort_device_flag(&fort_device()->conf, FORT_DEVICE_POWER_OFF) != 0)
        return FALSE;

    PFORT_FLOW flow = (PFORT_FLOW) ca->flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);
    const UCHAR speed_limit = ca->inbound ? FORT_FLOW_SPEED_LIMIT_IN : FORT_FLOW_SPEED_LIMIT_OUT;

    if ((flow_flags & speed_limit) == 0)
        return FALSE;

    ca->isIPv6 = (flow_flags & FORT_FLOW_IP6) != 0;

    /* Skip self injected packet */
    if (fort_packet_injected_by_self(ca))
        return FALSE;

    const NTSTATUS status = fort_shaper_packet_queue(shaper, ca, flow);

    return NT_SUCCESS(status);
}

FORT_API void fort_shaper_drop_flow_packets(PFORT_SHAPER shaper, UINT64 flowContext)
{
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    /* The active bits are cleared while the thread processes the queues, so check the flow's:
     * its current and previous (changed by a reauthorization) Speed Limits' queues */
    const FORT_SPEED_LIMIT_IDS speed_limits = flow->speed_limits;
    const FORT_SPEED_LIMIT_IDS old_speed_limits = flow->old_speed_limits;

    UINT32 flow_io_bits = fort_shaper_limit_bit(speed_limits.in_limit_id)
            | fort_shaper_limit_bit(speed_limits.out_limit_id)
            | fort_shaper_limit_bit(old_speed_limits.in_limit_id)
            | fort_shaper_limit_bit(old_speed_limits.out_limit_id);

    if (flow_io_bits == 0)
        return;

    /* Collect flow's packets from Queues */
    PFORT_FLOW_PACKET pkt_chain = NULL;

    for (int i = 0; flow_io_bits != 0; ++i) {
        const BOOL queue_exists = (flow_io_bits & 1) != 0;
        flow_io_bits >>= 1;

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
