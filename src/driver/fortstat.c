/* Fort Firewall Traffic Statistics */

#define FORT_STAT_POOL_TAG	'SwfF'

#define FORT_PROC_BAD_INDEX	((UINT16) -1)

typedef struct fort_stat_traf {
  UINT32 volatile in_bytes;
  UINT32 volatile out_bytes;
} FORT_STAT_TRAF, *PFORT_STAT_TRAF;

typedef struct fort_stat_proc {
  UINT16 refcount;

  UINT32 process_id;

  union {
    LARGE_INTEGER volatile traf_all;
    FORT_STAT_TRAF traf;
  };
} FORT_STAT_PROC, *PFORT_STAT_PROC;

typedef struct fort_stat {
  UINT16 id;

  UINT16 proc_count;
  UINT16 proc_top;
  UINT16 proc_end;
  UINT16 proc_free_index;

  PFORT_STAT_PROC procs;

  KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;


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

static void
fort_stat_proc_del (PFORT_STAT_PROC procs)
{
  ExFreePoolWithTag(procs, FORT_STAT_POOL_TAG);
}

static BOOL
fort_stat_proc_realloc (PFORT_STAT stat)
{
  const UINT16 proc_end = stat->proc_end;
  const UINT16 new_end = (proc_end ? proc_end : 8) * 3 / 2;
  PFORT_STAT_PROC new_procs = ExAllocatePoolWithTag(NonPagedPool,
    new_end * sizeof(FORT_STAT_PROC), FORT_STAT_POOL_TAG);

  if (new_procs == NULL)
    return FALSE;

  if (proc_end) {
    PFORT_STAT_PROC procs = stat->procs;

    RtlCopyMemory(new_procs, procs, stat->proc_top * sizeof(FORT_STAT_PROC));
    fort_stat_proc_del(procs);
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
fort_stat_init (PFORT_STAT stat)
{
  stat->proc_free_index = FORT_PROC_BAD_INDEX;

  KeInitializeSpinLock(&stat->lock);
}

static void
fort_stat_close (PFORT_STAT stat)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  if (stat->procs != NULL) {
    fort_stat_proc_del(stat->procs);

    stat->procs = NULL;

    stat->proc_count = 0;
    stat->proc_top = 0;
    stat->proc_end = 0;

    stat->proc_free_index = FORT_PROC_BAD_INDEX;

    stat->id++;
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static NTSTATUS
fort_stat_flow_associate (PFORT_STAT stat, UINT64 flow_id,
                          UINT32 callout_id, UINT32 process_id)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  UINT64 flow_context;
  UINT16 proc_index;
  BOOL is_new = FALSE;
  NTSTATUS status;

  FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_STREAM_V4, callout_id);

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

  flow_context = (UINT64) process_id
    | ((UINT64) proc_index << 32)
    | ((UINT64) stat->id << 48);

  status = FwpsFlowAssociateContext0(flow_id,
    FWPS_LAYER_STREAM_V4, callout_id, flow_context);

  if (NT_SUCCESS(status)) {
    fort_stat_proc_inc(stat, proc_index);
  }
  else if (is_new) {
    PFORT_STAT_PROC proc = &stat->procs[proc_index];

    fort_stat_proc_free(stat, proc, proc_index);
  }

 end:
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static void
fort_stat_flow_delete (PFORT_STAT stat, UINT64 flow_context)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  const UINT16 proc_index = (UINT16) (flow_context >> 32);
  const UINT16 stat_id = (UINT16) (flow_context >> 48);

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  if (stat_id == stat->id) {
    fort_stat_proc_dec(stat, proc_index);
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void
fort_stat_flow_classify (PFORT_STAT stat, UINT64 flow_id,
                         UINT32 callout_id, UINT64 flow_context,
                         UINT32 data_len, BOOL inbound)
{
  PFORT_STAT_PROC proc;
  KLOCK_QUEUE_HANDLE lock_queue;
  const UINT32 process_id = (UINT32) flow_context;
  const UINT16 proc_index = (UINT16) (flow_context >> 32);
  const UINT16 stat_id = (UINT16) (flow_context >> 48);
  BOOL is_old_flow = FALSE;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);
  if (stat_id == stat->id) {
    proc = &stat->procs[proc_index];

    if (inbound) {
      proc->traf.in_bytes += data_len;
    } else {
      proc->traf.out_bytes += data_len;
    }
  } else {
    is_old_flow = TRUE;
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  if (is_old_flow) {
    FwpsFlowRemoveContext0(flow_id, FWPS_LAYER_STREAM_V4, callout_id);
  }
}

static void
fort_stat_update (PFORT_STAT stat, const FORT_CONF_FLAGS conf_flags)
{
  if (!conf_flags.log_stat) {
    fort_stat_close(stat);
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
