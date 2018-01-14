/* Fort Firewall Traffic Statistics */

#define FORT_STAT_POOL_TAG	'SwfF'

#define FORT_PROC_BAD_INDEX	((UINT16) -1)

typedef struct fort_stat_traf {
  UINT32 in_bytes;
  UINT32 out_bytes;
} FORT_STAT_TRAF, *PFORT_STAT_TRAF;

typedef struct fort_stat_group {
  union {
    LARGE_INTEGER traf_all;
    FORT_STAT_TRAF traf;
  };
} FORT_STAT_GROUP, *PFORT_STAT_GROUP;

typedef struct fort_stat_proc {
  UINT16 next_index;
  UINT16 refcount;

  UINT32 process_id;

  union {
    LARGE_INTEGER traf_all;
    FORT_STAT_TRAF traf;
  };
} FORT_STAT_PROC, *PFORT_STAT_PROC;

typedef struct fort_stat_flow_opt {
  UCHAR speed_limit	: 1;
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

  UCHAR is_dirty	: 1;
  UCHAR log_stat	: 1;

  UINT16 limit_bits;

  UINT16 proc_count;
  UINT16 proc_top;
  UINT16 proc_end;

  UINT16 proc_head_index;
  UINT16 proc_free_index;

  UINT32 stream4_id;
  UINT32 datagram4_id;
  UINT32 in_transport4_id;
  UINT32 out_transport4_id;

  PFORT_STAT_PROC procs;

  PFORT_STAT_FLOW flow_free;

  tommy_arrayof flows;
  tommy_hashdyn flows_map;

  FORT_CONF_LIMIT limits[FORT_CONF_GROUP_MAX];
  FORT_STAT_GROUP groups[FORT_CONF_GROUP_MAX];

  KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;

#define fort_stat_flow_hash(flow_id)	tommy_inthash_u32((UINT32) (flow_id))

#define fort_stat_group_speed_limit(stat, group_index) \
  ((stat)->limit_bits & (1 << (group_index)))


static PVOID
fort_stat_array_new (SIZE_T size)
{
  return fort_mem_alloc(size, FORT_STAT_POOL_TAG);
}

static void
fort_stat_array_del (PVOID p)
{
  fort_mem_free(p, FORT_STAT_POOL_TAG);
}

static UINT16
fort_stat_proc_index (PFORT_STAT stat, UINT32 process_id)
{
  UINT16 proc_index = stat->proc_head_index;

  while (proc_index != FORT_PROC_BAD_INDEX) {
    PFORT_STAT_PROC proc = &stat->procs[proc_index];

    if (process_id == proc->process_id) {
      return proc_index;
    }

    proc_index = proc->next_index;
  }

  return FORT_PROC_BAD_INDEX;
}

static void
fort_stat_proc_exclude (PFORT_STAT stat, PFORT_STAT_PROC ex_proc,
                        UINT16 ex_proc_index)
{
  UINT16 proc_index = stat->proc_head_index;

  if (proc_index == ex_proc_index) {
    stat->proc_head_index = ex_proc->next_index;
  } else {
    do {
      PFORT_STAT_PROC proc = &stat->procs[proc_index];

      proc_index = proc->next_index;

      if (proc_index == ex_proc_index) {
        proc->next_index = ex_proc->next_index;
        return;
      }
    } while (proc_index != FORT_PROC_BAD_INDEX);
  }
}

static void
fort_stat_proc_free (PFORT_STAT stat, PFORT_STAT_PROC proc, UINT16 proc_index,
                     PFORT_STAT_PROC prev_proc /* = NULL */)
{
  /* Exclude from the active chain */
  if (prev_proc == NULL) {
    fort_stat_proc_exclude(stat, proc, proc_index);
  } else {
    prev_proc->next_index = proc->next_index;
  }

  if (proc_index == stat->proc_top - 1) {
    /* Chop from buffer */
    stat->proc_top--;
  } else {
    /* Add to free chain */
    proc->next_index = stat->proc_free_index;
    stat->proc_free_index = proc_index;
  }

  stat->proc_count--;
}

static void
fort_stat_proc_inc (PFORT_STAT stat, UINT16 proc_index)
{
  PFORT_STAT_PROC proc = &stat->procs[proc_index];

  ++proc->refcount;
}

static void
fort_stat_proc_dec (PFORT_STAT stat, UINT16 proc_index)
{
  PFORT_STAT_PROC proc = &stat->procs[proc_index];

  if (!--proc->refcount) {
    stat->is_dirty = TRUE;
  }
}

static BOOL
fort_stat_proc_realloc (PFORT_STAT stat)
{
  const UINT16 proc_end = stat->proc_end;
  const UINT16 new_end = (proc_end ? proc_end : 16) * 3 / 2;
  PFORT_STAT_PROC new_procs = fort_stat_array_new(
    new_end * sizeof(FORT_STAT_PROC));

  if (new_procs == NULL)
    return FALSE;

  if (proc_end) {
    PFORT_STAT_PROC procs = stat->procs;

    RtlCopyMemory(new_procs, procs, stat->proc_top * sizeof(FORT_STAT_PROC));
    fort_stat_array_del(procs);
  }

  stat->proc_end = new_end;
  stat->procs = new_procs;

  return TRUE;
}

static UINT16
fort_stat_proc_add (PFORT_STAT stat, UINT32 process_id)
{
  PFORT_STAT_PROC proc;
  UINT16 proc_index = stat->proc_free_index;

  if (proc_index != FORT_PROC_BAD_INDEX) {
    proc = &stat->procs[proc_index];

    stat->proc_free_index = proc->next_index;
  } else {
    if (stat->proc_top >= stat->proc_end
        && !fort_stat_proc_realloc(stat)) {
      return FORT_PROC_BAD_INDEX;
    }

    proc_index = stat->proc_top++;
    proc = &stat->procs[proc_index];
  }

  /* Prepend to active processes chain */
  proc->next_index = stat->proc_head_index;
  stat->proc_head_index = proc_index;

  proc->refcount = 0;
  proc->process_id = process_id;
  proc->traf_all.QuadPart = 0;

  stat->proc_count++;

  return proc_index;
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

      // TODO: tommy_arrayof_grow(): check calloc()'s result for NULL
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

  flow->opt.speed_limit = (UCHAR) speed_limit;
  flow->opt.group_index = group_index;
  flow->opt.proc_index = proc_index;

  return STATUS_SUCCESS;
}

static void
fort_stat_init (PFORT_STAT stat)
{
  stat->is_dirty = FALSE;

  if (stat->procs != NULL) {
    fort_stat_array_del(stat->procs);
    stat->procs = NULL;

    stat->proc_count = 0;
    stat->proc_top = 0;
    stat->proc_end = 0;
  }

  stat->proc_head_index = FORT_PROC_BAD_INDEX;
  stat->proc_free_index = FORT_PROC_BAD_INDEX;
}

static void
fort_stat_open (PFORT_STAT stat)
{
  fort_stat_init(stat);

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

  tommy_arrayof_done(&stat->flows);
  tommy_hashdyn_done(&stat->flows_map);

  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void
fort_stat_update (PFORT_STAT stat, BOOL log_stat)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  if (stat->log_stat) {
    tommy_hashdyn_foreach_node(&stat->flows_map, fort_stat_flow_close);

    fort_stat_init(stat);
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
  KLOCK_QUEUE_HANDLE lock_queue;
  UINT16 proc_index;
  BOOL speed_limit;
  NTSTATUS status;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  if (!stat->log_stat) {
    status = STATUS_RESOURCE_REQUIREMENTS_CHANGED;
    goto end;
  }

  proc_index = fort_stat_proc_index(stat, process_id);

  if (proc_index == FORT_PROC_BAD_INDEX) {
    proc_index = fort_stat_proc_add(stat, process_id);

    if (proc_index == FORT_PROC_BAD_INDEX) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto end;
    }

    *is_new_proc = TRUE;
  }

  speed_limit = fort_stat_group_speed_limit(stat, group_index) != 0;

  status = fort_stat_flow_add(stat, flow_id,
    group_index, proc_index, speed_limit);

  if (!NT_SUCCESS(status) && *is_new_proc) {
    PFORT_STAT_PROC proc = &stat->procs[proc_index];

    fort_stat_proc_free(stat, proc, proc_index, NULL);
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
    return;  // double check to avoid deadlock after remove-flow-context

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
    PFORT_STAT_PROC proc = &stat->procs[flow->opt.proc_index];
    UINT32 *proc_bytes = inbound ? &proc->traf.in_bytes
      : &proc->traf.out_bytes;

    // Add traffic to process
    *proc_bytes += data_len;

    if (flow->opt.speed_limit) {
      const UCHAR group_index = flow->opt.group_index;
      const PFORT_CONF_LIMIT group_limit = &stat->limits[group_index];
      const UINT32 limit_bytes = inbound ? group_limit->in_bytes
        : group_limit->out_bytes;

      if (limit_bytes != 0) {
        PFORT_STAT_GROUP group = &stat->groups[group_index];
        UINT32 *group_bytes = inbound ? &group->traf.in_bytes
          : &group->traf.out_bytes;

        if (*group_bytes < limit_bytes) {
          // Add traffic to app. group
          *group_bytes += data_len;
        } else {
          limited = TRUE;
        }
      }
    }

    stat->is_dirty = TRUE;
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
fort_stat_dpc_traf_flush (PFORT_STAT stat, PCHAR out)
{
  const UINT32 proc_bits_len = FORT_LOG_STAT_PROC_SIZE(stat->proc_count);
  PFORT_STAT_TRAF out_traf = (PFORT_STAT_TRAF) (out + proc_bits_len);
  PUCHAR out_proc_bits = (PUCHAR) out;
  UINT16 i;

  PFORT_STAT_PROC prev_proc = NULL;
  UINT16 proc_index = stat->proc_head_index;

  /* Mark processes as active to start */
  memset(out_proc_bits, 0xFF, proc_bits_len);

  for (i = 0; proc_index != FORT_PROC_BAD_INDEX; ++i) {
    PFORT_STAT_PROC proc = &stat->procs[proc_index];
    const UINT16 next_index = proc->next_index;

    /* Write bytes */
    *out_traf++ = proc->traf;

    if (!proc->refcount) {
      /* The process is inactive */
      out_proc_bits[i / 8] ^= (1 << (i & 7));

      fort_stat_proc_free(stat, proc, proc_index, prev_proc);
    } else {
      /* Clear active process's bytes */
      proc->traf_all.QuadPart = 0;

      prev_proc = proc;
    }

    proc_index = next_index;
  }

  if (stat->limit_bits) {
    PFORT_STAT_GROUP group = stat->groups;
    UINT16 limit_bits = stat->limit_bits;

    for (; limit_bits != 0; ++group) {
      if (limit_bits & 1) {
        group->traf_all.QuadPart = 0;
      }

      limit_bits >>= 1;
    }
  }

  stat->is_dirty = FALSE;
}
