/* Fort Firewall Traffic Statistics */

#include "fortstat.h"

#define FORT_STAT_POOL_TAG 'SwfF'

#define FORT_PROC_BAD_INDEX ((UINT16) -1)
#define FORT_PROC_COUNT_MAX 0x7FFF

#define fort_stat_proc_hash(process_id) tommy_inthash_u32((UINT32) (process_id))
#define fort_flow_hash(flow_id)         tommy_inthash_u32((UINT32) (flow_id))

static void fort_stat_proc_active_add(PFORT_STAT stat, PFORT_STAT_PROC proc)
{
    if (proc->active)
        return;

    proc->active = TRUE;

    /* Add to active chain */
    proc->next_active = stat->proc_active;
    stat->proc_active = proc;

    stat->proc_active_count++;
}

static void fort_stat_proc_active_clear(PFORT_STAT stat)
{
    stat->proc_active = NULL;
    stat->proc_active_count = 0;
}

static void fort_stat_proc_inc(PFORT_STAT stat, UINT16 proc_index)
{
    PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, proc_index);

    ++proc->refcount;
}

static void fort_stat_proc_dec(PFORT_STAT stat, UINT16 proc_index)
{
    PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, proc_index);

    if (--proc->refcount == 0) {
        fort_stat_proc_active_add(stat, proc);
    }
}

static PFORT_STAT_PROC fort_stat_proc_get(PFORT_STAT stat, UINT32 process_id, tommy_key_t proc_hash)
{
    PFORT_STAT_PROC proc = (PFORT_STAT_PROC) tommy_hashdyn_bucket(&stat->procs_map, proc_hash);

    while (proc != NULL) {
        if (proc->proc_hash == proc_hash && proc->process_id == process_id)
            return proc;

        proc = proc->next;
    }
    return NULL;
}

static void fort_stat_proc_free(PFORT_STAT stat, PFORT_STAT_PROC proc)
{
    tommy_hashdyn_remove_existing(&stat->procs_map, (tommy_hashdyn_node *) proc);

    /* Add to free chain */
    proc->next = stat->proc_free;
    stat->proc_free = proc;
}

static PFORT_STAT_PROC fort_stat_proc_add(PFORT_STAT stat, UINT32 process_id)
{
    const tommy_key_t proc_hash = fort_stat_proc_hash(process_id);
    PFORT_STAT_PROC proc;

    if (stat->proc_free != NULL) {
        proc = stat->proc_free;
        stat->proc_free = proc->next;
    } else {
        const tommy_size_t size = tommy_arrayof_size(&stat->procs);

        if (size + 1 >= FORT_PROC_COUNT_MAX)
            return NULL;

        /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
        if (tommy_arrayof_grow(&stat->procs, size + 1), 0)
            return NULL;

        proc = tommy_arrayof_ref(&stat->procs, size);

        proc->proc_index = (UINT16) size;
    }

    tommy_hashdyn_insert(&stat->procs_map, (tommy_hashdyn_node *) proc, 0, proc_hash);

    proc->active = FALSE;
    proc->refcount = 0;
    proc->process_id = process_id;
    proc->traf.v = 0;

    return proc;
}

FORT_API UCHAR fort_stat_flags_set(PFORT_STAT stat, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&stat->flags, flags) : InterlockedAnd8(&stat->flags, ~flags);
}

FORT_API UCHAR fort_stat_flags(PFORT_STAT stat)
{
    return fort_stat_flags_set(stat, 0, TRUE);
}

FORT_API UCHAR fort_flow_flags_set(PFORT_FLOW flow, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&flow->opt.flags, flags) : InterlockedAnd8(&flow->opt.flags, ~flags);
}

FORT_API UCHAR fort_flow_flags(PFORT_FLOW flow)
{
    return fort_flow_flags_set(flow, 0, TRUE);
}

static void fort_flow_context_stream_init(
        PFORT_STAT stat, BOOL isIPv6, BOOL is_tcp, UINT16 *layerId, UINT32 *calloutId)
{
    if (is_tcp) {
        if (isIPv6) {
            *layerId = FWPS_LAYER_STREAM_V6;
            *calloutId = stat->stream6_id;
        } else {
            *layerId = FWPS_LAYER_STREAM_V4;
            *calloutId = stat->stream4_id;
        }
    } else {
        if (isIPv6) {
            *layerId = FWPS_LAYER_DATAGRAM_DATA_V6;
            *calloutId = stat->datagram6_id;
        } else {
            *layerId = FWPS_LAYER_DATAGRAM_DATA_V4;
            *calloutId = stat->datagram4_id;
        }
    }
}

inline static void fort_flow_context_stream_set(
        PFORT_STAT stat, UINT64 flow_id, UINT64 flowContext, BOOL isIPv6, BOOL is_tcp)
{
    UINT16 layerId;
    UINT32 calloutId;

    fort_flow_context_stream_init(stat, isIPv6, is_tcp, &layerId, &calloutId);

    FwpsFlowAssociateContext0(flow_id, layerId, calloutId, flowContext);
}

inline static void fort_flow_context_transport_set(
        PFORT_STAT stat, UINT64 flow_id, UINT64 flowContext, BOOL isIPv6, BOOL inbound)
{
    if (isIPv6) {
        FwpsFlowAssociateContext0(
                flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V6, stat->in_transport6_id, flowContext);
        FwpsFlowAssociateContext0(
                flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V6, stat->out_transport6_id, flowContext);
    } else {
        FwpsFlowAssociateContext0(
                flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V4, stat->in_transport4_id, flowContext);
        FwpsFlowAssociateContext0(
                flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V4, stat->out_transport4_id, flowContext);
    }
}

static void fort_flow_context_set(
        PFORT_STAT stat, PFORT_FLOW flow, BOOL isIPv6, BOOL is_tcp, BOOL inbound)
{
    const UINT64 flow_id = flow->flow_id;
    const UINT64 flowContext = (UINT64) flow;

    fort_flow_context_stream_set(stat, flow_id, flowContext, isIPv6, is_tcp);
    fort_flow_context_transport_set(stat, flow_id, flowContext, isIPv6, inbound);
}

inline static BOOL fort_flow_context_stream_remove(
        PFORT_STAT stat, UINT64 flow_id, BOOL isIPv6, BOOL is_tcp)
{
    UINT16 layerId;
    UINT32 calloutId;

    fort_flow_context_stream_init(stat, isIPv6, is_tcp, &layerId, &calloutId);

    return FwpsFlowRemoveContext0(flow_id, layerId, calloutId) != STATUS_PENDING;
}

inline static NTSTATUS fort_flow_context_transport_remove(
        PFORT_STAT stat, UINT64 flow_id, BOOL isIPv6)
{
    NTSTATUS in_status;
    NTSTATUS out_status;

    if (isIPv6) {
        in_status = FwpsFlowRemoveContext0(
                flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V6, stat->in_transport6_id);
        out_status = FwpsFlowRemoveContext0(
                flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V6, stat->out_transport6_id);
    } else {
        in_status = FwpsFlowRemoveContext0(
                flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V4, stat->in_transport4_id);
        out_status = FwpsFlowRemoveContext0(
                flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V4, stat->out_transport4_id);
    }

    return in_status != STATUS_PENDING && out_status != STATUS_PENDING;
}

static BOOL fort_flow_context_remove_id(PFORT_STAT stat, UINT64 flow_id, BOOL isIPv6, BOOL is_tcp)
{
    const BOOL stream_res = fort_flow_context_stream_remove(stat, flow_id, isIPv6, is_tcp);

    const BOOL transport_res = fort_flow_context_transport_remove(stat, flow_id, isIPv6);

    return stream_res && transport_res;
}

static void fort_flow_context_remove(PFORT_STAT stat, PFORT_FLOW flow)
{
    const UINT64 flow_id = flow->flow_id;

    const UCHAR flow_flags = fort_flow_flags(flow);
    const BOOL is_tcp = (flow_flags & FORT_FLOW_TCP);
    const BOOL isIPv6 = (flow_flags & FORT_FLOW_IP6);

    if (!fort_flow_context_remove_id(stat, flow_id, isIPv6, is_tcp)) {
        fort_stat_flags_set(stat, FORT_STAT_FLOW_PENDING, TRUE);
    }
}

static BOOL fort_flow_is_closed(PFORT_FLOW flow)
{
    return (flow->opt.proc_index == FORT_PROC_BAD_INDEX);
}

static void fort_flow_close(PFORT_FLOW flow)
{
    flow->opt.proc_index = FORT_PROC_BAD_INDEX;
}

static PFORT_FLOW fort_flow_get(PFORT_STAT stat, UINT64 flow_id, tommy_key_t flow_hash)
{
    PFORT_FLOW flow = (PFORT_FLOW) tommy_hashdyn_bucket(&stat->flows_map, flow_hash);

    while (flow != NULL) {
        if (flow->flow_hash == flow_hash && flow->flow_id == flow_id)
            return flow;

        flow = flow->next;
    }
    return NULL;
}

static void fort_flow_free(PFORT_STAT stat, PFORT_FLOW flow)
{
    if (!fort_flow_is_closed(flow)) {
        fort_stat_proc_dec(stat, flow->opt.proc_index);
    }

    tommy_hashdyn_remove_existing(&stat->flows_map, (tommy_hashdyn_node *) flow);

    /* Add to free chain */
    flow->next = stat->flow_free;
    stat->flow_free = flow;
}

static PFORT_FLOW fort_flow_new(PFORT_STAT stat, UINT64 flow_id, const tommy_key_t flow_hash,
        BOOL isIPv6, BOOL is_tcp, BOOL inbound)
{
    PFORT_FLOW flow;

    if (stat->flow_free != NULL) {
        flow = stat->flow_free;
        stat->flow_free = flow->next;
    } else {
        const tommy_size_t size = tommy_arrayof_size(&stat->flows);

        /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
        if (tommy_arrayof_grow(&stat->flows, size + 1), 0)
            return NULL;

        flow = tommy_arrayof_ref(&stat->flows, size);
    }

    tommy_hashdyn_insert(&stat->flows_map, (tommy_hashdyn_node *) flow, 0, flow_hash);

    flow->flow_id = flow_id;

    fort_flow_context_set(stat, flow, isIPv6, is_tcp, inbound);

    return flow;
}

inline static UCHAR fort_stat_group_speed_limit(PFORT_CONF_GROUP conf_group, UCHAR group_index)
{
    if (((conf_group->group_bits & conf_group->limit_bits) & (1 << group_index)) == 0)
        return 0;

    return (((conf_group->limit_io_bits) >> (group_index * 2)) & 3);
}

static NTSTATUS fort_flow_add(PFORT_STAT stat, UINT64 flow_id, UCHAR group_index, UINT16 proc_index,
        BOOL isIPv6, BOOL is_tcp, BOOL inbound, BOOL is_reauth)
{
    const tommy_key_t flow_hash = fort_flow_hash(flow_id);
    PFORT_FLOW flow = fort_flow_get(stat, flow_id, flow_hash);
    BOOL is_new_flow = FALSE;

    if (flow == NULL) {
        if (is_reauth) {
            /* Remove existing flow context after reauth. to be able to associate a flow-context */
            if (!fort_flow_context_remove_id(stat, flow_id, isIPv6, is_tcp))
                return FORT_STATUS_FLOW_BLOCK;
        }

        flow = fort_flow_new(stat, flow_id, flow_hash, isIPv6, is_tcp, inbound);
        if (flow == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        is_new_flow = TRUE;
    } else {
        is_new_flow = fort_flow_is_closed(flow);
    }

    if (is_new_flow) {
        fort_stat_proc_inc(stat, proc_index);
    }

    const UCHAR speed_limit = fort_stat_group_speed_limit(&stat->conf_group, group_index);

    flow->opt.flags = speed_limit | (is_tcp ? FORT_FLOW_TCP : 0) | (isIPv6 ? FORT_FLOW_IP6 : 0)
            | (inbound ? FORT_FLOW_INBOUND : 0);
    flow->opt.group_index = group_index;
    flow->opt.proc_index = proc_index;

    return STATUS_SUCCESS;
}

FORT_API void fort_stat_open(PFORT_STAT stat)
{
    tommy_arrayof_init(&stat->procs, sizeof(FORT_STAT_PROC));
    tommy_hashdyn_init(&stat->procs_map);

    tommy_arrayof_init(&stat->flows, sizeof(FORT_FLOW));
    tommy_hashdyn_init(&stat->flows_map);

    KeInitializeSpinLock(&stat->lock);
}

static BOOL fort_stat_close_flows(PFORT_STAT stat)
{
    BOOL is_pending = FALSE;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
    {
        fort_stat_flags_set(stat, FORT_STAT_FLOW_PENDING, FALSE);

        tommy_hashdyn_foreach_node_arg(&stat->flows_map, &fort_flow_context_remove, stat);

        is_pending = (fort_stat_flags(stat) & FORT_STAT_FLOW_PENDING) != 0;
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return !is_pending;
}

FORT_API void fort_stat_close(PFORT_STAT stat)
{
    fort_stat_flags_set(stat, FORT_STAT_CLOSED, TRUE);

    while (!fort_stat_close_flows(stat)) {
        /* Wait for asynchronously deleting flows */
        LARGE_INTEGER delay;
        delay.QuadPart = -3000000; /* sleep 300000us (300ms) */

        KeDelayExecutionThread(KernelMode, FALSE, &delay);
    }

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    tommy_arrayof_done(&stat->procs);
    tommy_hashdyn_done(&stat->procs_map);

    tommy_arrayof_done(&stat->flows);
    tommy_hashdyn_done(&stat->flows_map);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void fort_stat_clear(PFORT_STAT stat)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    fort_stat_proc_active_clear(stat);

    tommy_hashdyn_foreach_node_arg(&stat->procs_map, &fort_stat_proc_free, stat);
    tommy_hashdyn_foreach_node(&stat->flows_map, &fort_flow_close);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API void fort_stat_log_update(PFORT_STAT stat, BOOL log_stat)
{
    const UCHAR old_stat_flags = fort_stat_flags_set(stat, FORT_STAT_LOG, log_stat);

    if (!log_stat && (old_stat_flags & FORT_STAT_LOG) != 0) {
        fort_stat_clear(stat);
    }
}

FORT_API void fort_stat_conf_update(PFORT_STAT stat, const PFORT_CONF_IO conf_io)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
    {
        stat->conf_group = conf_io->conf_group;
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API void fort_stat_conf_flags_update(PFORT_STAT stat, const PFORT_CONF_FLAGS conf_flags)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
    {
        stat->conf_group.group_bits = (UINT16) conf_flags->group_bits;
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static NTSTATUS fort_flow_associate_proc(
        PFORT_STAT stat, UINT32 process_id, BOOL *is_new_proc, PFORT_STAT_PROC *proc)
{
    if ((fort_stat_flags(stat) & FORT_STAT_LOG) == 0)
        return STATUS_DEVICE_DATA_ERROR;

    const tommy_key_t proc_hash = fort_stat_proc_hash(process_id);

    *proc = fort_stat_proc_get(stat, process_id, proc_hash);

    if (*proc == NULL) {
        *proc = fort_stat_proc_add(stat, process_id);

        if (*proc == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        *is_new_proc = TRUE;
    }

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS fort_flow_associate(PFORT_STAT stat, UINT64 flow_id, UINT32 process_id,
        UCHAR group_index, BOOL isIPv6, BOOL is_tcp, BOOL inbound, BOOL is_reauth,
        BOOL *is_new_proc)
{
    NTSTATUS status;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    PFORT_STAT_PROC proc = NULL;
    status = fort_flow_associate_proc(stat, process_id, is_new_proc, &proc);

    /* Add flow */
    if (NT_SUCCESS(status)) {
        status = fort_flow_add(
                stat, flow_id, group_index, proc->proc_index, isIPv6, is_tcp, inbound, is_reauth);

        if (!NT_SUCCESS(status) && *is_new_proc) {
            fort_stat_proc_free(stat, proc);
        }
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return status;
}

FORT_API void fort_flow_delete(PFORT_STAT stat, UINT64 flowContext)
{
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    if ((fort_stat_flags(stat) & FORT_STAT_CLOSED) != 0)
        return; /* double check to avoid deadlock after remove-flow-context */

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
    if ((fort_stat_flags(stat) & FORT_STAT_CLOSED) == 0) {
        fort_flow_free(stat, flow);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API void fort_flow_classify(PFORT_STAT stat, UINT64 flowContext, UINT32 data_len, BOOL inbound)
{
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    if ((fort_stat_flags(stat) & FORT_STAT_LOG) != 0 && !fort_flow_is_closed(flow)) {
        PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, flow->opt.proc_index);
        UINT32 *proc_bytes = inbound ? &proc->traf.in_bytes : &proc->traf.out_bytes;

        /* Add traffic to process */
        *proc_bytes += data_len;

        fort_stat_proc_active_add(stat, proc);
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API void fort_stat_dpc_begin(PFORT_STAT stat, PKLOCK_QUEUE_HANDLE lock_queue)
{
    KeAcquireInStackQueuedSpinLockAtDpcLevel(&stat->lock, lock_queue);
}

FORT_API void fort_stat_dpc_end(PKLOCK_QUEUE_HANDLE lock_queue)
{
    KeReleaseInStackQueuedSpinLockFromDpcLevel(lock_queue);
}

FORT_API void fort_stat_dpc_traf_flush(PFORT_STAT stat, UINT16 proc_count, PCHAR out)
{
    PFORT_STAT_PROC proc = stat->proc_active;

    while (proc != NULL && proc_count-- != 0) {
        PFORT_STAT_PROC proc_next = proc->next_active;
        UINT32 *out_proc = (UINT32 *) out;
        PFORT_TRAF out_traf = (PFORT_TRAF) (out_proc + 1);

        out = (PCHAR) (out_traf + 1);

        /* Write bytes */
        *out_traf = proc->traf;

        /* Write process_id */
        *out_proc = proc->process_id;

        if (proc->refcount == 0) {
            /* The process is inactive */
            *out_proc |= 1;

            fort_stat_proc_free(stat, proc);
        } else {
            proc->active = FALSE;

            /* Clear process's bytes */
            proc->traf.v = 0;
        }

        proc = proc_next;

        stat->proc_active_count--;
    }

    stat->proc_active = proc;
}
