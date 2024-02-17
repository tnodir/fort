/* Fort Firewall Traffic Statistics */

#include "fortstat.h"

#define FORT_STAT_POOL_TAG 'SwfF'

#define FORT_PROC_BAD_INDEX ((UINT16) -1)
#define FORT_PROC_COUNT_MAX 0xFFFF

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

static PFORT_STAT_PROC fort_stat_proc_get(PFORT_STAT stat, UINT32 process_id, tommy_key_t pid_hash)
{
    PFORT_STAT_PROC proc = (PFORT_STAT_PROC) tommy_hashdyn_bucket(&stat->procs_map, pid_hash);

    while (proc != NULL) {
        if (proc->process_id == process_id)
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
    const tommy_key_t pid_hash = fort_stat_proc_hash(process_id);
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

    tommy_hashdyn_insert(&stat->procs_map, (tommy_hashdyn_node *) proc, 0, pid_hash);

    proc->process_id = process_id;
    proc->traf.v = 0;
    proc->log_stat = FALSE;
    proc->active = FALSE;
    proc->refcount = 0;

    return proc;
}

static void fort_stat_proc_unlog(PVOID proc_node)
{
    PFORT_STAT_PROC proc = proc_node;

    proc->log_stat = FALSE;
}

static void fort_stat_proc_inc(PFORT_STAT stat, UINT16 proc_index)
{
    PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, proc_index);

    ++proc->refcount;
}

static void fort_stat_proc_dec(PFORT_STAT stat, UINT16 proc_index)
{
    PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, proc_index);

    if (--proc->refcount != 0 || proc->active)
        return;

    if (proc->log_stat) {
        fort_stat_proc_active_add(stat, proc);
    } else {
        /* The process is terminated */
        fort_stat_proc_free(stat, proc);
    }
}

FORT_API UCHAR fort_stat_flags_set(PFORT_STAT stat, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&stat->flags, flags) : InterlockedAnd8(&stat->flags, ~flags);
}

FORT_API UCHAR fort_stat_flags(PFORT_STAT stat)
{
    return fort_stat_flags_set(stat, 0, TRUE);
}

inline static UINT32 fort_stat_callout_id(
        PFORT_STAT stat, enum FORT_STAT_CALLOUT_ID_TYPE calloutIdType)
{
    return stat->callout_ids[calloutIdType];
}

FORT_API UCHAR fort_flow_flags_set(PFORT_FLOW flow, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&flow->opt.flags, flags) : InterlockedAnd8(&flow->opt.flags, ~flags);
}

FORT_API UCHAR fort_flow_flags(PFORT_FLOW flow)
{
    return fort_flow_flags_set(flow, 0, TRUE);
}

inline static void fort_flow_context_stream_init_stream(
        PFORT_STAT stat, BOOL isIPv6, UINT16 *layerId, UINT32 *calloutId)
{
    if (isIPv6) {
        *layerId = FWPS_LAYER_STREAM_V6;
        *calloutId = fort_stat_callout_id(stat, FORT_STAT_STREAM6_ID);
    } else {
        *layerId = FWPS_LAYER_STREAM_V4;
        *calloutId = fort_stat_callout_id(stat, FORT_STAT_STREAM4_ID);
    }
}

inline static void fort_flow_context_stream_init_datagram(
        PFORT_STAT stat, BOOL isIPv6, UINT16 *layerId, UINT32 *calloutId)
{
    if (isIPv6) {
        *layerId = FWPS_LAYER_DATAGRAM_DATA_V6;
        *calloutId = fort_stat_callout_id(stat, FORT_STAT_DATAGRAM6_ID);
    } else {
        *layerId = FWPS_LAYER_DATAGRAM_DATA_V4;
        *calloutId = fort_stat_callout_id(stat, FORT_STAT_DATAGRAM4_ID);
    }
}

static void fort_flow_context_stream_init(
        PFORT_STAT stat, BOOL isIPv6, BOOL is_tcp, UINT16 *layerId, UINT32 *calloutId)
{
    if (is_tcp) {
        fort_flow_context_stream_init_stream(stat, isIPv6, layerId, calloutId);
    } else {
        fort_flow_context_stream_init_datagram(stat, isIPv6, layerId, calloutId);
    }
}

inline static NTSTATUS fort_flow_context_stream_set(
        PFORT_STAT stat, UINT64 flow_id, UINT64 flowContext, BOOL isIPv6, BOOL is_tcp)
{
    UINT16 layerId;
    UINT32 calloutId;

    fort_flow_context_stream_init(stat, isIPv6, is_tcp, &layerId, &calloutId);

    return FwpsFlowAssociateContext0(flow_id, layerId, calloutId, flowContext);
}

inline static void fort_flow_context_transport_set(
        PFORT_STAT stat, UINT64 flow_id, UINT64 flowContext, BOOL isIPv6, BOOL inbound)
{
    if (isIPv6) {
        FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V6,
                fort_stat_callout_id(stat, FORT_STAT_IN_TRANSPORT6_ID), flowContext);
        FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V6,
                fort_stat_callout_id(stat, FORT_STAT_OUT_TRANSPORT6_ID), flowContext);
    } else {
        FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V4,
                fort_stat_callout_id(stat, FORT_STAT_IN_TRANSPORT4_ID), flowContext);
        FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V4,
                fort_stat_callout_id(stat, FORT_STAT_OUT_TRANSPORT4_ID), flowContext);
    }
}

static NTSTATUS fort_flow_context_set(
        PFORT_STAT stat, PFORT_FLOW flow, BOOL isIPv6, BOOL is_tcp, BOOL inbound)
{
    const UINT64 flow_id = flow->flow_id;
    const UINT64 flowContext = (UINT64) flow;

    const NTSTATUS status =
            fort_flow_context_stream_set(stat, flow_id, flowContext, isIPv6, is_tcp);
    if (!NT_SUCCESS(status))
        return status;

    fort_flow_context_transport_set(stat, flow_id, flowContext, isIPv6, inbound);

    return STATUS_SUCCESS;
}

inline static void fort_flow_context_stream_remove(
        PFORT_STAT stat, UINT64 flow_id, BOOL isIPv6, BOOL is_tcp, NTSTATUS *status)
{
    UINT16 layerId;
    UINT32 calloutId;

    fort_flow_context_stream_init(stat, isIPv6, is_tcp, &layerId, &calloutId);

    *status = FwpsFlowRemoveContext0(flow_id, layerId, calloutId);
}

inline static void fort_flow_context_transport_remove(
        PFORT_STAT stat, UINT64 flow_id, BOOL isIPv6, NTSTATUS *in_status, NTSTATUS *out_status)
{
    if (isIPv6) {
        *in_status = FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V6,
                fort_stat_callout_id(stat, FORT_STAT_IN_TRANSPORT6_ID));
        *out_status = FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V6,
                fort_stat_callout_id(stat, FORT_STAT_OUT_TRANSPORT6_ID));
    } else {
        *in_status = FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V4,
                fort_stat_callout_id(stat, FORT_STAT_IN_TRANSPORT4_ID));
        *out_status = FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V4,
                fort_stat_callout_id(stat, FORT_STAT_OUT_TRANSPORT4_ID));
    }
}

static BOOL fort_flow_context_remove_id(
        PFORT_STAT stat, UINT64 flow_id, BOOL isIPv6, BOOL is_tcp, BOOL *pending)
{
    NTSTATUS stream_status;
    fort_flow_context_stream_remove(stat, flow_id, isIPv6, is_tcp, &stream_status);

    NTSTATUS transport_in_status;
    NTSTATUS transport_out_status;
    fort_flow_context_transport_remove(
            stat, flow_id, isIPv6, &transport_in_status, &transport_out_status);

    *pending = (stream_status == STATUS_PENDING || transport_in_status == STATUS_PENDING
            || transport_out_status == STATUS_PENDING);

    return stream_status == 0 && transport_in_status == 0 && transport_out_status == 0;
}

static void fort_flow_context_remove(PVOID stat_arg, PVOID flow_node)
{
    PFORT_STAT stat = stat_arg;
    PFORT_FLOW flow = flow_node;

    const UINT64 flow_id = flow->flow_id;

    const UCHAR flow_flags = fort_flow_flags(flow);
    const BOOL is_tcp = (flow_flags & FORT_FLOW_TCP);
    const BOOL isIPv6 = (flow_flags & FORT_FLOW_IP6);

    BOOL pending = FALSE;

    if (!fort_flow_context_remove_id(stat, flow_id, isIPv6, is_tcp, &pending)) {
#if !defined(FORT_WIN7_COMPAT)
        if (!pending) {
            /* The flow has associated context, but FwpsFlowRemoveContext0()
             * returns that there is no context as STATUS_UNSUCCESSFUL. */
            FwpsFlowAbort0(flow_id);
        }
#endif
    }
}

static PFORT_FLOW fort_flow_get(PFORT_STAT stat, UINT64 flow_id, tommy_key_t flow_hash)
{
    PFORT_FLOW flow = (PFORT_FLOW) tommy_hashdyn_bucket(&stat->flows_map, flow_hash);

    while (flow != NULL) {
        if (flow->flow_id == flow_id)
            return flow;

        flow = flow->next;
    }
    return NULL;
}

static void fort_flow_free(PFORT_STAT stat, PFORT_FLOW flow)
{
    fort_stat_proc_dec(stat, flow->opt.proc_index);

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

    tommy_hashdyn_insert(&stat->flows_map, (tommy_hashdyn_node *) flow, NULL, flow_hash);

    flow->flow_id = flow_id;

    return flow;
}

inline static UCHAR fort_stat_group_speed_limit(PFORT_CONF_GROUP conf_group, UCHAR group_index)
{
    if (((conf_group->group_bits & conf_group->limit_bits) & (1 << group_index)) == 0)
        return 0;

    return (((conf_group->limit_io_bits) >> (group_index * 2)) & 3);
}

inline static NTSTATUS fort_flow_add_new(PFORT_STAT stat, PFORT_FLOW *flow, UINT64 flow_id,
        tommy_key_t flow_hash, BOOL isIPv6, BOOL is_tcp, BOOL inbound, BOOL is_reauth)
{
    *flow = fort_flow_new(stat, flow_id, flow_hash, isIPv6, is_tcp, inbound);
    if (*flow == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    NTSTATUS status = fort_flow_context_set(stat, *flow, isIPv6, is_tcp, inbound);
    if (!NT_SUCCESS(status)) {
        fort_flow_free(stat, *flow);

        /* Can't remove existing context, because of possible deadlock */
        status = is_reauth ? FORT_STATUS_FLOW_BLOCK : status;
    }

    return status;
}

static NTSTATUS fort_flow_add(PFORT_STAT stat, UINT64 flow_id, UCHAR group_index, UINT16 proc_index,
        BOOL isIPv6, BOOL is_tcp, BOOL inbound, BOOL is_reauth)
{
    const tommy_key_t flow_hash = fort_flow_hash(flow_id);
    PFORT_FLOW flow = fort_flow_get(stat, flow_id, flow_hash);

    if (flow == NULL) {
        const NTSTATUS status = fort_flow_add_new(
                stat, &flow, flow_id, flow_hash, isIPv6, is_tcp, inbound, is_reauth);

        if (!NT_SUCCESS(status))
            return status;

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

FORT_API void fort_stat_close_flows(PFORT_STAT stat)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
    {
        const UCHAR flags = fort_stat_flags_set(stat, FORT_STAT_LOG, FALSE);

        if ((flags & FORT_STAT_CLOSED) == 0) {
            fort_stat_flags_set(stat, FORT_STAT_CLOSED, TRUE);

            InterlockedAdd(&stat->flow_closing_count, (LONG) tommy_hashdyn_count(&stat->flows_map));
        }
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    while (InterlockedAdd(&stat->flow_closing_count, 0) > 0) {
        KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
        {
            tommy_hashdyn_foreach_node_arg(&stat->flows_map, &fort_flow_context_remove, stat);
        }
        KeReleaseInStackQueuedSpinLock(&lock_queue);

        /* Wait for asynchronously deleting flows */
        LARGE_INTEGER delay = {
            .QuadPart = -50 * 1000 * 10 /* sleep 50000us (50ms) */
        };

        KeDelayExecutionThread(KernelMode, FALSE, &delay);
    }
}

FORT_API void fort_stat_close(PFORT_STAT stat)
{
    fort_stat_close_flows(stat);

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    tommy_arrayof_done(&stat->procs);
    tommy_hashdyn_done(&stat->procs_map);

    tommy_arrayof_done(&stat->flows);
    tommy_hashdyn_done(&stat->flows_map);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API void fort_stat_log_update(PFORT_STAT stat, BOOL log_stat)
{
    const UCHAR old_stat_flags = fort_stat_flags_set(stat, FORT_STAT_LOG, log_stat);

    if (log_stat || (old_stat_flags & FORT_STAT_LOG) == 0)
        return;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    /* Clear the processes' active list */
    fort_stat_traf_flush(stat, /*proc_count=*/FORT_PROC_COUNT_MAX, /*out=*/NULL);

    /* Clear the processes' logged flag */
    tommy_hashdyn_foreach_node(&stat->procs_map, &fort_stat_proc_unlog);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
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

    const tommy_key_t pid_hash = fort_stat_proc_hash(process_id);

    *proc = fort_stat_proc_get(stat, process_id, pid_hash);

    if (*proc == NULL) {
        *proc = fort_stat_proc_add(stat, process_id);

        if (*proc == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        *is_new_proc = TRUE;
    }

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS fort_flow_associate(PFORT_STAT stat, UINT64 flow_id, UINT32 process_id,
        UCHAR group_index, BOOL isIPv6, BOOL is_tcp, BOOL inbound, BOOL is_reauth, BOOL *log_stat)
{
    NTSTATUS status;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    BOOL is_new_proc = FALSE;
    PFORT_STAT_PROC proc = NULL;
    status = fort_flow_associate_proc(stat, process_id, &is_new_proc, &proc);

    /* Add flow */
    if (NT_SUCCESS(status)) {
        status = fort_flow_add(
                stat, flow_id, group_index, proc->proc_index, isIPv6, is_tcp, inbound, is_reauth);

        if (NT_SUCCESS(status)) {
            *log_stat = proc->log_stat;
            proc->log_stat = TRUE;
        } else if (is_new_proc) {
            fort_stat_proc_free(stat, proc);
        }
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return status;
}

static BOOL fort_flow_delete_closing(PFORT_STAT stat)
{
    if ((fort_stat_flags(stat) & FORT_STAT_CLOSED) != 0) {
        InterlockedDecrement(&stat->flow_closing_count);
        return TRUE;
    }
    return FALSE;
}

FORT_API void fort_flow_delete(PFORT_STAT stat, UINT64 flowContext)
{
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    if (fort_flow_delete_closing(stat))
        return;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    /* Double check for locked flows */
    if (!fort_flow_delete_closing(stat)) {
        fort_flow_free(stat, flow);
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API void fort_flow_classify(PFORT_STAT stat, UINT64 flowContext, UINT32 data_len, BOOL inbound)
{
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

    PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, flow->opt.proc_index);

    if (proc->log_stat) {
        UINT32 *proc_bytes = inbound ? &proc->traf.in_bytes : &proc->traf.out_bytes;

        /* Add traffic to process's bytes */
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

static void fort_stat_traf_flush_proc(PFORT_STAT stat, PFORT_STAT_PROC proc, PCHAR *out)
{
    PUINT32 out_proc = (PUINT32) *out;
    PFORT_TRAF out_traf = (PFORT_TRAF) (out_proc + 1);

    *out = (PCHAR) (out_traf + 1);

    /* Write bytes */
    *out_traf = proc->traf;

    /* Write process_id */
    *out_proc = proc->process_id
            /* The process is terminated */
            | (proc->refcount == 0 ? 1 : 0);
}

FORT_API void fort_stat_traf_flush(PFORT_STAT stat, UINT16 proc_count, PCHAR out)
{
    PFORT_STAT_PROC proc = stat->proc_active;

    for (; proc != NULL && proc_count != 0; --proc_count) {
        PFORT_STAT_PROC proc_next = proc->next_active;

        if (out != NULL) {
            fort_stat_traf_flush_proc(stat, proc, &out);
        }

        if (proc->refcount == 0) {
            /* The process is terminated */
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
