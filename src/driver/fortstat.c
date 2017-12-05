/* Fort Firewall Traffic Statistics */

#define FORT_STAT_POOL_TAG	'SwfF'

#define FORT_PROC_BAD_INDEX	((UINT16) -1)
#define FORT_FLOW_BAD_INDEX	((UINT32) -1)

typedef struct fort_stat_traf {
  UINT32 in_bytes;
  UINT32 out_bytes;
} FORT_STAT_TRAF, *PFORT_STAT_TRAF;

typedef struct fort_stat_proc {
  UINT16 refcount;

  UINT32 process_id;

  union {
    LARGE_INTEGER traf_all;
    FORT_STAT_TRAF traf;
  };
} FORT_STAT_PROC, *PFORT_STAT_PROC;

typedef struct fort_stat_flow {
  union {
    LARGE_INTEGER l;
    UINT64 flow_id;
  };
} FORT_STAT_FLOW, *PFORT_STAT_FLOW;

typedef struct fort_stat {
  UINT16 volatile closing;

  UINT16 version;

  UINT16 proc_count;
  UINT16 proc_top;
  UINT16 proc_end;
  UINT16 proc_free_index;

  UINT32 flow_count;
  UINT32 flow_top;
  UINT32 flow_end;
  UINT32 flow_free_index;

  PFORT_STAT_PROC procs;
  PFORT_STAT_FLOW flows;

  KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;


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
  PFORT_STAT_PROC proc = stat->procs;
  const UINT16 proc_top = stat->proc_top;
  UINT16 i;

  for (i = 0; i < proc_top; ++i, ++proc) {
    if (process_id == proc->process_id)
      return i;
  }

  return FORT_PROC_BAD_INDEX;
}

static void
fort_stat_proc_free (PFORT_STAT stat, PFORT_STAT_PROC proc, UINT16 proc_index)
{
  if (proc_index == stat->proc_top - 1) {
    /* Chop from buffer */
    stat->proc_top--;
  } else {
    /* Add to free chain */
    proc->process_id = 0;

    proc->refcount = stat->proc_free_index;
    stat->proc_free_index = proc_index;
  }

  stat->proc_count--;
}

static void
fort_stat_proc_inc (PFORT_STAT stat, UINT16 proc_index)
{
  PFORT_STAT_PROC proc = &stat->procs[proc_index];

  proc->refcount++;
}

static void
fort_stat_proc_dec (PFORT_STAT stat, UINT16 proc_index)
{
  PFORT_STAT_PROC proc = &stat->procs[proc_index];

  proc->refcount--;
}

static BOOL
fort_stat_proc_realloc (PFORT_STAT stat)
{
  const UINT16 proc_end = stat->proc_end;
  const UINT16 new_end = (proc_end ? proc_end : 8) * 3 / 2;
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

    stat->proc_free_index = proc->refcount;
  } else {
    if (stat->proc_top >= stat->proc_end
        && !fort_stat_proc_realloc(stat)) {
      return FORT_PROC_BAD_INDEX;
    }

    proc_index = stat->proc_top++;
    proc = &stat->procs[proc_index];
  }

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
    /* Add to free chain */
    PFORT_STAT_FLOW flow = &stat->flows[flow_index];

    flow->l.HighPart = FORT_FLOW_BAD_INDEX;
    flow->l.LowPart = stat->flow_free_index;
    stat->flow_free_index = flow_index;
  }

  stat->flow_count--;
}

static BOOL
fort_stat_flow_realloc (PFORT_STAT stat)
{
  const UINT32 flow_end = stat->flow_end;
  const UINT32 new_end = (flow_end ? flow_end : 8) * 2;
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
fort_stat_flow_add (PFORT_STAT stat, UINT64 flow_id)
{
  PFORT_STAT_FLOW flow;
  UINT32 flow_index = stat->flow_free_index;

  if (flow_index != FORT_FLOW_BAD_INDEX) {
    flow = &stat->flows[flow_index];

    stat->flow_free_index = flow->l.LowPart;
  } else {
    if (stat->flow_top >= stat->flow_end
        && !fort_stat_flow_realloc(stat)) {
      return FORT_FLOW_BAD_INDEX;
    }

    flow_index = stat->flow_top++;
    flow = &stat->flows[flow_index];
  }

  flow->flow_id = flow_id;

  stat->flow_count++;

  return flow_index;
}

static NTSTATUS
fort_stat_flow_set_context (UINT64 flow_id, UINT32 callout_id,
                            UINT64 flow_context)
{
  return FwpsFlowAssociateContext0(flow_id, FWPS_LAYER_STREAM_V4,
    callout_id, flow_context);
}

static void
fort_stat_flow_remove_context (UINT64 flow_id, UINT32 callout_id)
{
  FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_STREAM_V4, callout_id);
}

static void
fort_stat_init (PFORT_STAT stat)
{
  stat->proc_free_index = FORT_PROC_BAD_INDEX;
  stat->flow_free_index = FORT_FLOW_BAD_INDEX;

  KeInitializeSpinLock(&stat->lock);
}

static void
fort_stat_close (PFORT_STAT stat, UINT32 callout_id)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  stat->closing = TRUE;
  stat->version++;

  if (stat->procs != NULL) {
    fort_stat_array_del(stat->procs);
    stat->procs = NULL;

    stat->proc_count = 0;
    stat->proc_top = 0;
    stat->proc_end = 0;

    stat->proc_free_index = FORT_PROC_BAD_INDEX;
  }

  if (stat->flows != NULL) {
    // Remove flow contexts
    {
      PFORT_STAT_FLOW flow = stat->flows;
      UINT32 count = stat->flow_count;

      for (; count != 0; ++flow) {
        if (flow->l.HighPart == FORT_FLOW_BAD_INDEX)
          continue;

        fort_stat_flow_remove_context(flow->flow_id, callout_id);

        --count;
      }
    }

    fort_stat_array_del(stat->flows);
    stat->flows = NULL;

    stat->flow_count = 0;
    stat->flow_top = 0;
    stat->flow_end = 0;

    stat->flow_free_index = FORT_FLOW_BAD_INDEX;
  }

  stat->closing = FALSE;
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static NTSTATUS
fort_stat_flow_associate (PFORT_STAT stat, UINT64 flow_id,
                          UINT32 callout_id, UINT32 process_id)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  UINT64 flow_context;
  UINT32 flow_index;
  UINT16 proc_index;
  BOOL is_new = FALSE;
  NTSTATUS status;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  proc_index = fort_stat_proc_index(stat, process_id);

  if (proc_index == FORT_PROC_BAD_INDEX) {
    proc_index = fort_stat_proc_add(stat, process_id);

    if (proc_index == FORT_PROC_BAD_INDEX) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto end;
    }

    is_new = TRUE;
  }

  flow_index = fort_stat_flow_add(stat, flow_id);

  if (flow_index == FORT_FLOW_BAD_INDEX) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto cleanup;
  }

  flow_context = (UINT64) flow_index
    | ((UINT64) proc_index << 32)
    | ((UINT64) stat->version << 48);

  status = fort_stat_flow_set_context(flow_id, callout_id, flow_context);

  if (NT_SUCCESS(status)) {
    fort_stat_proc_inc(stat, proc_index);
  }

 cleanup:
  if (!NT_SUCCESS(status)) {
    fort_stat_flow_free(stat, flow_index);

    if (is_new) {
      PFORT_STAT_PROC proc = &stat->procs[proc_index];

      fort_stat_proc_free(stat, proc, proc_index);
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
  const UINT32 flow_index = (UINT32) (flow_context >> 32);
  const UINT16 proc_index = (UINT16) (flow_context >> 32);
  const UINT16 stat_version = (UINT16) (flow_context >> 48);

  if (stat->closing)
    return;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  if (stat_version == stat->version) {
    fort_stat_flow_free(stat, flow_index);
    fort_stat_proc_dec(stat, proc_index);
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void
fort_stat_flow_classify (PFORT_STAT stat, UINT64 flow_context,
                         UINT32 data_len, BOOL inbound)
{
  PFORT_STAT_PROC proc;
  KLOCK_QUEUE_HANDLE lock_queue;
  const UINT16 proc_index = (UINT16) (flow_context >> 32);
  const UINT16 stat_version = (UINT16) (flow_context >> 48);

  if (stat->closing)
    return;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  if (stat_version == stat->version) {
    proc = &stat->procs[proc_index];

    if (inbound) {
      proc->traf.in_bytes += data_len;
    } else {
      proc->traf.out_bytes += data_len;
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void
fort_stat_update (PFORT_STAT stat, const FORT_CONF_FLAGS conf_flags,
                  UINT32 callout_id)
{
  if (!conf_flags.log_stat) {
    fort_stat_close(stat, callout_id);
  }
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
fort_stat_dpc_traf_flush (PFORT_STAT stat, UINT16 proc_index, UINT16 count,
                          PCHAR out)
{
  const UINT32 proc_bits_len = FORT_LOG_STAT_PROC_SIZE(count);
  PFORT_STAT_PROC proc = &stat->procs[proc_index];
  PFORT_STAT_TRAF out_traf = (PFORT_STAT_TRAF) (out + proc_bits_len);
  PUCHAR out_proc_bits = (PUCHAR) out;

  /* All processes are active */
  memset(out_proc_bits, 0xFF, proc_bits_len);

  for (; count != 0; ++proc_index, ++proc) {
    if (!proc->process_id)
      continue;

    *out_traf = proc->traf;

    if (!proc->refcount) {
      /* The process is inactive */
      out_proc_bits[proc_index / 8] ^= (1 << (proc_index & 7));

      fort_stat_proc_free(stat, proc, proc_index);
    } else {
      proc->traf_all.QuadPart = 0;
    }

    --count;
  }
}
