/* Fort Firewall Traffic Statistics */

#define FORT_STAT_POOL_TAG	'SwfF'

#define FORT_PROC_BAD_INDEX	((UINT16) -1)
#define FORT_FLOW_BAD_INDEX	((UINT32) -1)
#define FORT_FLOW_BAD_ID	((UINT64) -1LL)

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

typedef struct fort_stat_flow {
  UINT32 is_udp		: 1;
  UINT32 speed_limit	: 1;

  UINT32 next_free_index;

  UINT64 flow_id;
} FORT_STAT_FLOW, *PFORT_STAT_FLOW;

typedef struct fort_stat {
  UCHAR volatile closing;
  UCHAR is_dirty;

  UCHAR version;

  UINT16 limit_bits;

  UINT16 proc_count;
  UINT16 proc_top;
  UINT16 proc_end;

  UINT16 proc_head_index;
  UINT16 proc_free_index;

  UINT32 flow_count;
  UINT32 flow_top;
  UINT32 flow_end;

  UINT32 flow_free_index;

  UINT32 stream4_id;
  UINT32 datagram4_id;
  UINT32 in_transport4_id;
  UINT32 out_transport4_id;

  FORT_CONF_LIMIT limits[FORT_CONF_GROUP_MAX];
  FORT_STAT_GROUP groups[FORT_CONF_GROUP_MAX];

  PFORT_STAT_PROC procs;
  PFORT_STAT_FLOW flows;

  KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;

#define fort_stat_group_speed_limit(stat, group_index) \
  ((stat)->limit_bits & (1 << (group_index)))


static void
fort_stat_array_del (PVOID p)
{
  ExFreePoolWithTag(p, FORT_STAT_POOL_TAG);
}

static PVOID
fort_stat_array_new (SIZE_T size)
{
  return ExAllocatePoolWithTag(NonPagedPool, size, FORT_STAT_POOL_TAG);
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
fort_stat_flow_free (PFORT_STAT stat, UINT32 flow_index)
{
  if (flow_index == stat->flow_top - 1) {
    /* Chop from buffer */
    stat->flow_top--;
  } else {
    PFORT_STAT_FLOW flow = &stat->flows[flow_index];

    /* Add to free chain */
    flow->flow_id = FORT_FLOW_BAD_ID;

    flow->next_free_index = stat->flow_free_index;
    stat->flow_free_index = flow_index;
  }

  stat->flow_count--;
}

static BOOL
fort_stat_flow_realloc (PFORT_STAT stat)
{
  const UINT32 flow_end = stat->flow_end;
  const UINT32 new_end = (flow_end ? flow_end : 64) * 2;
  PFORT_STAT_FLOW new_flows = fort_stat_array_new(
    new_end * sizeof(FORT_STAT_FLOW));

  if (new_flows == NULL)
    return FALSE;

  if (flow_end) {
    PFORT_STAT_FLOW flows = stat->flows;

    RtlCopyMemory(new_flows, flows, stat->flow_top * sizeof(FORT_STAT_FLOW));
    fort_stat_array_del(flows);
  }

  stat->flow_end = new_end;
  stat->flows = new_flows;

  return TRUE;
}

static UINT32
fort_stat_flow_add (PFORT_STAT stat, UINT64 flow_id,
                    BOOL is_udp, BOOL speed_limit)
{
  PFORT_STAT_FLOW flow;
  UINT32 flow_index = stat->flow_free_index;

  if (flow_index != FORT_FLOW_BAD_INDEX) {
    flow = &stat->flows[flow_index];

    stat->flow_free_index = flow->next_free_index;
  } else {
    if (stat->flow_top >= stat->flow_end
        && !fort_stat_flow_realloc(stat)) {
      return FORT_FLOW_BAD_INDEX;
    }

    flow_index = stat->flow_top++;
    flow = &stat->flows[flow_index];
  }

  flow->is_udp = is_udp;
  flow->speed_limit = speed_limit;

  flow->flow_id = flow_id;

  stat->flow_count++;

  return flow_index;
}

static NTSTATUS
fort_stat_flow_set_context (UINT64 flow_id, UINT16 layer_id,
                            UINT32 callout_id, UINT64 flow_context)
{
  return FwpsFlowAssociateContext0(flow_id, layer_id,
    callout_id, flow_context);
}

static NTSTATUS
fort_stat_flow_remove_context (UINT64 flow_id, UINT16 layer_id,
                               UINT32 callout_id)
{
  return FwpsFlowRemoveContext0(flow_id, layer_id, callout_id);
}

static void
fort_stat_flow_transport_set_contexts (PFORT_STAT stat, UINT64 flow_id,
                                       UINT64 flow_context, BOOL is_udp,
                                       BOOL speed_limit, BOOL is_reauth)
{
  if (is_udp) return;

  if (is_reauth) {
    fort_stat_flow_remove_context(flow_id,
      FWPS_LAYER_INBOUND_TRANSPORT_V4, stat->in_transport4_id);
    fort_stat_flow_remove_context(flow_id,
      FWPS_LAYER_OUTBOUND_TRANSPORT_V4, stat->out_transport4_id);
  }

  if (speed_limit) {
    fort_stat_flow_set_context(flow_id, FWPS_LAYER_INBOUND_TRANSPORT_V4,
      stat->in_transport4_id, flow_context);
    fort_stat_flow_set_context(flow_id, FWPS_LAYER_OUTBOUND_TRANSPORT_V4,
      stat->out_transport4_id, flow_context);
  }
}

static void
fort_stat_flow_transport_remove_contexts (PFORT_STAT stat, PFORT_STAT_FLOW flow)
{
  if (!flow->is_udp && flow->speed_limit) {
    fort_stat_flow_remove_context(flow->flow_id,
      FWPS_LAYER_INBOUND_TRANSPORT_V4, stat->in_transport4_id);
    fort_stat_flow_remove_context(flow->flow_id,
      FWPS_LAYER_OUTBOUND_TRANSPORT_V4, stat->out_transport4_id);
  }
}

static void
fort_stat_flow_remove_contexts (PFORT_STAT stat)
{
  PFORT_STAT_FLOW flow = stat->flows;
  UINT32 count = stat->flow_count;

  for (; count != 0; ++flow) {
    if (flow->flow_id == FORT_FLOW_BAD_ID)
      continue;

    // Remove stream/dgram layer context
    if (flow->is_udp) {
      fort_stat_flow_remove_context(flow->flow_id,
        FWPS_LAYER_DATAGRAM_DATA_V4, stat->datagram4_id);
    } else {
      fort_stat_flow_remove_context(flow->flow_id,
        FWPS_LAYER_STREAM_V4, stat->stream4_id);
    }

    // Remove stream transport layer contexts
    fort_stat_flow_transport_remove_contexts(stat, flow);

    --count;
  }
}

static void
fort_stat_init_indexes (PFORT_STAT stat)
{
  stat->proc_head_index = FORT_PROC_BAD_INDEX;
  stat->proc_free_index = FORT_PROC_BAD_INDEX;

  stat->flow_free_index = FORT_FLOW_BAD_INDEX;

  while (!++stat->version)
    continue;  /* version must not be zero to avoid zero flow-context */
}

static void
fort_stat_init (PFORT_STAT stat)
{
  fort_stat_init_indexes(stat);

  KeInitializeSpinLock(&stat->lock);
}

static void
fort_stat_close (PFORT_STAT stat)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  stat->closing = TRUE;
  stat->is_dirty = FALSE;

  if (stat->procs != NULL) {
    fort_stat_array_del(stat->procs);
    stat->procs = NULL;

    stat->proc_count = 0;
    stat->proc_top = 0;
    stat->proc_end = 0;
  }

  if (stat->flows != NULL) {
    fort_stat_flow_remove_contexts(stat);

    fort_stat_array_del(stat->flows);
    stat->flows = NULL;

    stat->flow_count = 0;
    stat->flow_top = 0;
    stat->flow_end = 0;
  }

  fort_stat_init_indexes(stat);

  stat->closing = FALSE;

  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void
fort_stat_update (PFORT_STAT stat, const FORT_CONF_FLAGS conf_flags)
{
  if (!conf_flags.log_stat) {
    fort_stat_close(stat);
  }
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
                          BOOL is_udp, BOOL is_reauth, BOOL *is_new)
{
  const UINT16 layer_id = is_udp ? FWPS_LAYER_DATAGRAM_DATA_V4
    : FWPS_LAYER_STREAM_V4;
  const UINT32 callout_id = is_udp ? stat->datagram4_id
    : stat->stream4_id;

  KLOCK_QUEUE_HANDLE lock_queue;
  UINT64 flow_context;
  UINT32 flow_index;
  UINT16 proc_index;
  BOOL speed_limit;
  NTSTATUS status;

  if (flow_id == FORT_FLOW_BAD_ID)
    return STATUS_INVALID_PARAMETER;

  if (is_reauth) {
    fort_stat_flow_remove_context(flow_id, layer_id, callout_id);
  }

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  proc_index = fort_stat_proc_index(stat, process_id);

  if (proc_index == FORT_PROC_BAD_INDEX) {
    proc_index = fort_stat_proc_add(stat, process_id);

    if (proc_index == FORT_PROC_BAD_INDEX) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto end;
    }

    *is_new = TRUE;
  }

  speed_limit = fort_stat_group_speed_limit(stat, group_index);

  flow_index = fort_stat_flow_add(stat, flow_id, is_udp, speed_limit);

  if (flow_index == FORT_FLOW_BAD_INDEX) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto cleanup;
  }

  flow_context = (UINT64) flow_index
    | ((UINT64) stat->version << 32)
    | ((UINT64) group_index << 40)
    | ((UINT64) proc_index << 48);

  // Set stream/dgram layer context
  status = fort_stat_flow_set_context(
    flow_id, layer_id, callout_id, flow_context);

  if (NT_SUCCESS(status)) {
    // Set stream transport layer contexts
    fort_stat_flow_transport_set_contexts(
      stat, flow_id, flow_context, is_udp, speed_limit, is_reauth);

    fort_stat_proc_inc(stat, proc_index);
  } else {
    fort_stat_flow_free(stat, flow_index);

 cleanup:
    if (*is_new) {
      PFORT_STAT_PROC proc = &stat->procs[proc_index];

      fort_stat_proc_free(stat, proc, proc_index, NULL);

      *is_new = FALSE;
    }
  }

 end:
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static void
fort_stat_flow_delete (PFORT_STAT stat, UINT64 flow_context)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  const UINT32 flow_index = (UINT32) flow_context;
  const UINT16 proc_index = (UINT16) (flow_context >> 48);
  const UCHAR stat_version = (UCHAR) (flow_context >> 32);

  if (stat->closing)
    return;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  if (stat_version == stat->version) {
    PFORT_STAT_FLOW flow = &stat->flows[flow_index];

    fort_stat_flow_transport_remove_contexts(stat, flow);
    fort_stat_flow_free(stat, flow_index);
    fort_stat_proc_dec(stat, proc_index);
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static BOOL
fort_stat_flow_classify (PFORT_STAT stat, UINT64 flow_context,
                         UINT32 data_len, BOOL inbound)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  const UINT16 proc_index = (UINT16) (flow_context >> 48);
  const UCHAR group_index = (UCHAR) (flow_context >> 40);
  const UCHAR stat_version = (UCHAR) (flow_context >> 32);
  BOOL limited = FALSE;

  if (stat->closing)
    return FALSE;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  if (stat_version == stat->version) {
    PFORT_STAT_PROC proc = &stat->procs[proc_index];
    UINT32 *proc_bytes = inbound ? &proc->traf.in_bytes
      : &proc->traf.out_bytes;

    // Add traffic to process
    *proc_bytes += data_len;

    if (fort_stat_group_speed_limit(stat, group_index)) {
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
      /* Zero active process's bytes */
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
