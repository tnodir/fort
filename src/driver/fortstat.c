/* Fort Firewall Traffic Statistics */

#define FORT_STAT_POOL_TAG	'SwfF'

#define FORT_PROC_BAD_INDEX	((UINT16) -1)
#define FORT_PROC_COUNT_MAX	0x7FFF

typedef struct fort_stat_traf {
  union {
    struct {
      UINT32 in_bytes;
      UINT32 out_bytes;
    };

    UINT64 v;
  };
} FORT_STAT_TRAF, *PFORT_STAT_TRAF;

typedef struct fort_stat_group {
  FORT_STAT_TRAF traf;
} FORT_STAT_GROUP, *PFORT_STAT_GROUP;

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_stat_proc {
  struct fort_stat_proc *next;
  struct fort_stat_proc *prev;

  union {
#if defined(_WIN64)
    FORT_STAT_TRAF traf;
#else
    UINT32 process_id;
#endif
    void *data;
  };

  tommy_key_t proc_hash;

  UINT16 proc_index	: 15;  /* Synchronize with FORT_PROC_COUNT_MAX! */
  UINT16 active		: 1;

  UINT16 refcount;

#if defined(_WIN64)
  UINT32 process_id;
#else
  FORT_STAT_TRAF traf;
#endif

  struct fort_stat_proc *next_active;
} FORT_STAT_PROC, *PFORT_STAT_PROC;

#define FORT_STAT_FLOW_SPEED_LIMIT	0x01
#define FORT_STAT_FLOW_DEFER_IN		0x02
#define FORT_STAT_FLOW_DEFER_OUT	0x04

typedef struct fort_stat_flow_opt {
  UCHAR volatile flags;
  UCHAR group_index;
  UINT16 proc_index;
} FORT_STAT_FLOW_OPT, *PFORT_STAT_FLOW_OPT;

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_stat_flow {
  struct fort_stat_flow *next;
  struct fort_stat_flow *prev;

  union {
#if defined(_WIN64)
    UINT64 flow_id;
#else
    FORT_STAT_FLOW_OPT opt;
#endif
    void *data;
  };

  tommy_key_t flow_hash;

#if defined(_WIN64)
  FORT_STAT_FLOW_OPT opt;
#else
  UINT64 flow_id;
#endif
} FORT_STAT_FLOW, *PFORT_STAT_FLOW;

typedef struct fort_stat {
  UCHAR volatile closed;

  UCHAR log_stat	: 1;

  UINT16 limit_bits;
  UINT16 proc_active_count;

  UINT32 stream4_id;
  UINT32 datagram4_id;
  UINT32 in_transport4_id;
  UINT32 out_transport4_id;

  PFORT_STAT_PROC proc_free;
  PFORT_STAT_PROC proc_active;

  PFORT_STAT_FLOW flow_free;

  tommy_arrayof procs;
  tommy_hashdyn procs_map;

  tommy_arrayof flows;
  tommy_hashdyn flows_map;

  FORT_CONF_LIMIT limits[FORT_CONF_GROUP_MAX];
  FORT_STAT_GROUP groups[FORT_CONF_GROUP_MAX];

  KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;

#define fort_stat_proc_hash(process_id)	tommy_inthash_u32((UINT32) (process_id))
#define fort_stat_flow_hash(flow_id)	tommy_inthash_u32((UINT32) (flow_id))

#define fort_stat_group_speed_limit(stat, group_index) \
  ((stat)->limit_bits & (1 << (group_index)))


static void
fort_stat_proc_active_add (PFORT_STAT stat, PFORT_STAT_PROC proc)
{
  if (proc->active)
    return;

  proc->active = TRUE;

  /* Add to active chain */
  proc->next_active = stat->proc_active;
  stat->proc_active = proc;

  stat->proc_active_count++;
}

static void
fort_stat_proc_active_clear (PFORT_STAT stat)
{
  stat->proc_active = NULL;
  stat->proc_active_count = 0;
}

static void
fort_stat_proc_inc (PFORT_STAT stat, UINT16 proc_index)
{
  PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, proc_index);

  ++proc->refcount;
}

static void
fort_stat_proc_dec (PFORT_STAT stat, UINT16 proc_index)
{
  PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, proc_index);

  if (!--proc->refcount) {
    fort_stat_proc_active_add(stat, proc);
  }
}

static PFORT_STAT_PROC
fort_stat_proc_get (PFORT_STAT stat, UINT32 process_id, tommy_key_t proc_hash)
{
  PFORT_STAT_PROC proc = (PFORT_STAT_PROC) tommy_hashdyn_bucket(
    &stat->procs_map, proc_hash);

  while (proc != NULL) {
    if (proc->proc_hash == proc_hash && proc->process_id == process_id)
      return proc;

    proc = proc->next;
  }
  return NULL;
}

static void
fort_stat_proc_free (PFORT_STAT stat, PFORT_STAT_PROC proc)
{
  tommy_hashdyn_remove_existing(&stat->procs_map, (tommy_hashdyn_node *) proc);

  /* Add to free chain */
  proc->next = stat->proc_free;
  stat->proc_free = proc;
}

static PFORT_STAT_PROC
fort_stat_proc_add (PFORT_STAT stat, UINT32 process_id)
{
  const tommy_key_t proc_hash = fort_stat_proc_hash(process_id);
  PFORT_STAT_PROC proc;

  if (stat->proc_free != NULL) {
    proc = stat->proc_free;
    stat->proc_free = proc->next;
  } else {
    const tommy_count_t size = tommy_arrayof_size(&stat->procs);

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

static UCHAR
fort_stat_flow_flags (PFORT_STAT_FLOW flow)
{
  return flow->opt.flags;
}

static void
fort_stat_flow_flags_set (PFORT_STAT_FLOW flow, UCHAR flags)
{
  flow->opt.flags |= flags;
}

static void
fort_stat_flow_flags_clear (PFORT_STAT_FLOW flow, UCHAR flags)
{
  flow->opt.flags &= ~flags;
}

static void
fort_stat_flow_context_set (PFORT_STAT stat, PFORT_STAT_FLOW flow)
{
  const UINT64 flow_id = flow->flow_id;
  const UINT64 flowContext = (UINT64) flow;

  FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_STREAM_V4, stat->stream4_id, flowContext);
  FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_DATAGRAM_DATA_V4, stat->datagram4_id, flowContext);
  FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V4, stat->in_transport4_id, flowContext);
  FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V4, stat->out_transport4_id, flowContext);
}

static void
fort_stat_flow_context_remove (PFORT_STAT stat, PFORT_STAT_FLOW flow)
{
  const UINT64 flow_id = flow->flow_id;

  FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_STREAM_V4, stat->stream4_id);
  FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_DATAGRAM_DATA_V4, stat->datagram4_id);
  FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V4, stat->in_transport4_id);
  FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V4, stat->out_transport4_id);
}

static void
fort_stat_flow_close (PFORT_STAT_FLOW flow)
{
  flow->opt.proc_index = FORT_PROC_BAD_INDEX;
}

static PFORT_STAT_FLOW
fort_stat_flow_get (PFORT_STAT stat, UINT64 flow_id, tommy_key_t flow_hash)
{
  PFORT_STAT_FLOW flow = (PFORT_STAT_FLOW) tommy_hashdyn_bucket(
    &stat->flows_map, flow_hash);

  while (flow != NULL) {
    if (flow->flow_hash == flow_hash && flow->flow_id == flow_id)
      return flow;

    flow = flow->next;
  }
  return NULL;
}

static void
fort_stat_flow_free (PFORT_STAT stat, PFORT_STAT_FLOW flow)
{
  const UINT16 proc_index = flow->opt.proc_index;

  if (proc_index != FORT_PROC_BAD_INDEX) {
    fort_stat_proc_dec(stat, proc_index);
  }

  tommy_hashdyn_remove_existing(&stat->flows_map, (tommy_hashdyn_node *) flow);

  /* Add to free chain */
  flow->next = stat->flow_free;
  stat->flow_free = flow;
}

static NTSTATUS
fort_stat_flow_add (PFORT_STAT stat, UINT64 flow_id,
                    UCHAR group_index, UINT16 proc_index,
                    BOOL speed_limit)
{
  const tommy_key_t flow_hash = fort_stat_flow_hash(flow_id);
  PFORT_STAT_FLOW flow = fort_stat_flow_get(stat, flow_id, flow_hash);
  BOOL is_new_flow = FALSE;

  if (flow == NULL) {
    if (stat->flow_free != NULL) {
      flow = stat->flow_free;
      stat->flow_free = flow->next;
    } else {
      const tommy_count_t size = tommy_arrayof_size(&stat->flows);

      /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
      if (tommy_arrayof_grow(&stat->flows, size + 1), 0)
        return STATUS_INSUFFICIENT_RESOURCES;

      flow = tommy_arrayof_ref(&stat->flows, size);
    }

    tommy_hashdyn_insert(&stat->flows_map, (tommy_hashdyn_node *) flow, 0, flow_hash);

    flow->flow_id = flow_id;

    fort_stat_flow_context_set(stat, flow);

    is_new_flow = TRUE;
  } else {
    is_new_flow = (flow->opt.proc_index == FORT_PROC_BAD_INDEX);
  }

  if (is_new_flow) {
    fort_stat_proc_inc(stat, proc_index);
  }

  flow->opt.flags = (speed_limit ? FORT_STAT_FLOW_SPEED_LIMIT : 0);
  flow->opt.group_index = group_index;
  flow->opt.proc_index = proc_index;

  return STATUS_SUCCESS;
}

static void
fort_stat_open (PFORT_STAT stat)
{
  tommy_arrayof_init(&stat->procs, sizeof(FORT_STAT_PROC));
  tommy_hashdyn_init(&stat->procs_map);

  tommy_arrayof_init(&stat->flows, sizeof(FORT_STAT_FLOW));
  tommy_hashdyn_init(&stat->flows_map);

  KeInitializeSpinLock(&stat->lock);
}

static void
fort_stat_close (PFORT_STAT stat)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  stat->closed = TRUE;

  tommy_hashdyn_foreach_node_arg(&stat->flows_map,
    fort_stat_flow_context_remove, stat);

  tommy_arrayof_done(&stat->procs);
  tommy_hashdyn_done(&stat->procs_map);

  tommy_arrayof_done(&stat->flows);
  tommy_hashdyn_done(&stat->flows_map);

  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void
fort_stat_clear (PFORT_STAT stat)
{
  fort_stat_proc_active_clear(stat);

  tommy_hashdyn_foreach_node_arg(&stat->procs_map, fort_stat_proc_free, stat);
  tommy_hashdyn_foreach_node(&stat->flows_map, fort_stat_flow_close);
}

static void
fort_stat_update (PFORT_STAT stat, BOOL log_stat)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  if (stat->log_stat && !log_stat) {
    fort_stat_clear(stat);
  }

  stat->log_stat = (UCHAR) log_stat;

  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void
fort_stat_update_limits (PFORT_STAT stat, PFORT_CONF_IO conf_io)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  {
    const UINT16 limit_bits = conf_io->limit_bits;

    stat->limit_bits = limit_bits;

    if (limit_bits != 0) {
      RtlCopyMemory(stat->limits, conf_io->limits, sizeof(stat->limits));
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static NTSTATUS
fort_stat_flow_associate (PFORT_STAT stat, UINT64 flow_id,
                          UINT32 process_id, UCHAR group_index,
                          BOOL *is_new_proc)
{
  const tommy_key_t proc_hash = fort_stat_proc_hash(process_id);
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_STAT_PROC proc;
  BOOL speed_limit;
  NTSTATUS status;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  if (!stat->log_stat) {
    status = STATUS_RESOURCE_REQUIREMENTS_CHANGED;
    goto end;
  }

  proc = fort_stat_proc_get(stat, process_id, proc_hash);

  if (proc == NULL) {
    proc = fort_stat_proc_add(stat, process_id);

    if (proc == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto end;
    }

    *is_new_proc = TRUE;
  }

  speed_limit = fort_stat_group_speed_limit(stat, group_index) != 0;

  status = fort_stat_flow_add(stat, flow_id,
    group_index, proc->proc_index, speed_limit);

  if (!NT_SUCCESS(status) && *is_new_proc) {
    fort_stat_proc_free(stat, proc);
  }

 end:
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static void
fort_stat_flow_delete (PFORT_STAT stat, UINT64 flowContext)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_STAT_FLOW flow = (PFORT_STAT_FLOW) flowContext;

  if (stat->closed)
    return;  /* double check to avoid deadlock after remove-flow-context */

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  if (!stat->closed) {
    fort_stat_flow_free(stat, flow);
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static BOOL
fort_stat_flow_classify (PFORT_STAT stat, UINT64 flowContext,
                         UINT32 data_len, BOOL inbound)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_STAT_FLOW flow = (PFORT_STAT_FLOW) flowContext;
  BOOL limited = FALSE;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  if (stat->log_stat && flow->opt.proc_index != FORT_PROC_BAD_INDEX) {
    PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat->procs, flow->opt.proc_index);
    UINT32 *proc_bytes = inbound ? &proc->traf.in_bytes
      : &proc->traf.out_bytes;

    /* Add traffic to process */
    *proc_bytes += data_len;

    if (flow->opt.flags & FORT_STAT_FLOW_SPEED_LIMIT) {
      const UCHAR group_index = flow->opt.group_index;
      const PFORT_CONF_LIMIT group_limit = &stat->limits[group_index];
      const UINT32 limit_bytes = inbound ? group_limit->in_bytes
        : group_limit->out_bytes;

      if (limit_bytes != 0) {
        PFORT_STAT_GROUP group = &stat->groups[group_index];
        UINT32 *group_bytes = inbound ? &group->traf.in_bytes
          : &group->traf.out_bytes;
        BOOL defer_flow = TRUE;

        if (*group_bytes < limit_bytes) {
          /* Add traffic to app. group */
          *group_bytes += data_len;

          defer_flow = (*group_bytes >= limit_bytes);
        } else {
          limited = TRUE;
        }

        /* Defer ACK */
        {
          const UCHAR defer_flag = inbound
            ? FORT_STAT_FLOW_DEFER_OUT : FORT_STAT_FLOW_DEFER_IN;

          if (defer_flow)
            fort_stat_flow_flags_set(flow, defer_flag);
          else
            fort_stat_flow_flags_clear(flow, defer_flag);
        }
      }
    }

    fort_stat_proc_active_add(stat, proc);
  }

  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return limited;
}

static void
fort_stat_dpc_begin (PFORT_STAT stat, PKLOCK_QUEUE_HANDLE lock_queue)
{
  KeAcquireInStackQueuedSpinLockAtDpcLevel(&stat->lock, lock_queue);
}

static void
fort_stat_dpc_end (PKLOCK_QUEUE_HANDLE lock_queue)
{
  KeReleaseInStackQueuedSpinLockFromDpcLevel(lock_queue);
}

static void
fort_stat_dpc_traf_flush (PFORT_STAT stat, UINT16 proc_count, PCHAR out)
{
  PFORT_STAT_PROC proc;

  proc = stat->proc_active;

  while (proc != NULL && proc_count-- != 0) {
    PFORT_STAT_PROC proc_next = proc->next_active;
    UINT32 *out_proc = (UINT32 *) out;
    PFORT_STAT_TRAF out_traf = (PFORT_STAT_TRAF) (out_proc + 1);

    out = (PCHAR) (out_traf + 1);

    /* Write bytes */
    *out_traf = proc->traf;

    /* Write process_id */
    *out_proc = proc->process_id;

    if (!proc->refcount) {
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

static void
fort_stat_dpc_group_flush (PFORT_STAT stat)
{
  /* Clear process group's bytes */
  if (stat->limit_bits) {
    PFORT_STAT_GROUP group = stat->groups;
    UINT16 limit_bits = stat->limit_bits;

    for (; limit_bits != 0; ++group) {
      if (limit_bits & 1) {
        group->traf.v = 0;
      }

      limit_bits >>= 1;
    }
  }
}
