/* Fort Firewall Traffic Statistics */

#define FORT_STAT_POOL_TAG	'SwfF'

typedef struct fort_stat_traf {
  UINT32 volatile in_bytes;
  UINT32 volatile out_bytes;
} FORT_STAT_TRAF, *PFORT_STAT_TRAF;

typedef struct fort_stat_proc {
  UINT32 process_id;

  union {
    LARGE_INTEGER volatile traf_all;
    FORT_STAT_TRAF traf;
  };

  int refcount;
} FORT_STAT_PROC, *PFORT_STAT_PROC;

typedef struct fort_stat {
  UINT16 proc_top;
  UINT16 proc_end;
  UINT16 proc_count;

  int proc_free_index;

  PFORT_STAT_PROC procs;

  KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;


static int
fort_stat_proc_index (PFORT_STAT stat, UINT32 process_id)
{
  PFORT_STAT_PROC proc = stat->procs;
  const int n = stat->proc_top;
  int i;

  for (i = 0; i < n; ++i, ++proc) {
    if (process_id == proc->process_id)
      return i;
  }

  return -1;
}

static void
fort_stat_proc_inc (PFORT_STAT stat, int proc_index)
{
  PFORT_STAT_PROC proc = &stat->procs[proc_index];

  proc->refcount++;
}

static NTSTATUS
fort_stat_proc_dec (PFORT_STAT stat, int proc_index)
{
  PFORT_STAT_PROC proc = &stat->procs[proc_index];

  if (--proc->refcount > 0)
    return STATUS_OBJECT_NAME_EXISTS;

  if (proc_index == stat->proc_top - 1) {
    /* Chop from buffer */
    stat->proc_top--;
  } else {
    /* Add to free chain */
    proc->process_id = 0;

    proc->refcount = stat->proc_free_index;
    stat->proc_free_index = proc_index;

    stat->proc_count--;
  }

  return STATUS_SUCCESS;
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
  const UINT16 new_end = (proc_end ? proc_end : 16) * 3 / 2;
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

static int
fort_stat_proc_add (PFORT_STAT stat, UINT32 process_id)
{
  PFORT_STAT_PROC proc;
  int proc_index = stat->proc_free_index;

  if (proc_index != -1) {
    proc = &stat->procs[proc_index];

    stat->proc_free_index = proc->refcount;
  } else {
    if (stat->proc_top >= stat->proc_end
        && !fort_stat_proc_realloc(stat)) {
      return -1;
    }

    proc_index = stat->proc_top++;
    proc = &stat->procs[proc_index];
  }

  proc->process_id = process_id;
  proc->traf_all.QuadPart = 0;
  proc->refcount = 1;

  stat->proc_count++;

  return proc_index;
}

static void
fort_stat_init (PFORT_STAT stat)
{
  stat->proc_free_index = -1;

  KeInitializeSpinLock(&stat->lock);
}

static void
fort_stat_close (PFORT_STAT stat)
{
  if (stat->procs != NULL) {
    fort_stat_proc_del(stat->procs);
  }
}

static NTSTATUS
fort_stat_flow_associate (PFORT_STAT stat, UINT64 flow_id,
                          UINT32 callout_id, UINT32 process_id)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  UINT64 flow_context;
  int proc_index;
  NTSTATUS status;

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  proc_index = fort_stat_proc_index(stat, process_id);
  if (proc_index == -1) {
    proc_index = fort_stat_proc_add(stat, process_id);
    if (proc_index == -1) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto end;
    }
  }

  flow_context = (UINT64) process_id | ((UINT64) proc_index << 32);

  fort_stat_proc_inc(stat, proc_index);

  status = FwpsFlowAssociateContext0(flow_id,
    FWPS_LAYER_STREAM_V4, callout_id, flow_context);

  if (!NT_SUCCESS(status)) {
    fort_stat_proc_dec(stat, proc_index);
  }

 end:
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow +: %d\n", process_id);

  return status;
}

static NTSTATUS
fort_stat_flow_delete (PFORT_STAT stat, UINT64 flow_context)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  NTSTATUS status;
  const int proc_index = (int) (flow_context >> 32);

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  status = fort_stat_proc_dec(stat, proc_index);

  KeReleaseInStackQueuedSpinLock(&lock_queue);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow -: %d\n", (UINT32) flow_context);

  return status;
}

static void
fort_stat_flow_classify (PFORT_STAT stat, UINT64 flow_context,
                         UINT32 data_len, BOOL inbound)
{
  PFORT_STAT_PROC proc;
  KLOCK_QUEUE_HANDLE lock_queue;
  const UINT32 process_id = (UINT32) flow_context;
  const int proc_index = (int) (flow_context >> 32);

  KeAcquireInStackQueuedSpinLock(&stat->lock, &lock_queue);

  proc = &stat->procs[proc_index];

  if (inbound) {
    proc->traf.in_bytes += data_len;
  } else {
    proc->traf.out_bytes += data_len;
  }

  KeReleaseInStackQueuedSpinLock(&lock_queue);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow >: %d %d %d\n", (UINT32) flow_context, data_len, inbound);
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
fort_stat_dpc_traf_write (PFORT_STAT stat, UINT32 index, UINT32 count,
                          PCHAR out)
{
  PFORT_STAT_PROC proc = &stat->procs[index];
  PFORT_STAT_TRAF out_traf = (PFORT_STAT_TRAF) out;

  for (; index < count; ++proc) {
    if (!proc->process_id)
      continue;

    *out_traf = proc->traf;

    ++index;
  }
}
