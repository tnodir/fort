/* Fort Firewall Traffic Statistics */

#define FORT_STAT_POOL_TAG	'SwfF'

typedef struct fort_stat_proc {
  UINT32 process_id;

  UINT32 in_bytes;
  UINT32 out_bytes;

  int refcount;
} FORT_STAT_PROC, *PFORT_STAT_PROC;

typedef struct fort_stat {
  UINT32 proc_top;
  UINT32 proc_count;

  int proc_free_index;

  PFORT_STAT_PROC procs;

  KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;


static int
fort_stat_proc_index (PFORT_STAT stat, UINT32 process_id)
{
  PFORT_STAT_PROC proc = stat->procs;
  const int n = stat->proc_count;
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

static void
fort_stat_proc_dec (PFORT_STAT stat, int proc_index)
{
  PFORT_STAT_PROC proc = &stat->procs[proc_index];

  if (--proc->refcount > 0)
    return;

  proc->process_id = 0;

  proc->refcount = stat->proc_free_index;
  stat->proc_free_index = proc_index;
}

static void
fort_stat_proc_del (PFORT_STAT_PROC procs)
{
  ExFreePoolWithTag(procs, FORT_STAT_POOL_TAG);
}

static BOOL
fort_stat_proc_realloc (PFORT_STAT stat)
{
  const UINT32 count = stat->proc_count;
  const UINT32 new_count = (count ? count : 16) * 3 / 2;
  PFORT_STAT_PROC new_procs = ExAllocatePoolWithTag(NonPagedPool,
    new_count * sizeof(FORT_STAT_PROC), FORT_STAT_POOL_TAG);

  if (new_procs == NULL)
    return FALSE;

  if (count) {
    PFORT_STAT_PROC procs = stat->procs;

    RtlCopyMemory(new_procs, procs, stat->proc_top * sizeof(FORT_STAT_PROC));
    fort_stat_proc_del(procs);
  }

  stat->proc_count = new_count;
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
  }
  else {
    if (stat->proc_top >= stat->proc_count
        && !fort_stat_proc_realloc(stat)) {
      return -1;
    }

    proc_index = stat->proc_top++;
    proc = &stat->procs[proc_index];
  }

  proc->process_id = process_id;
  proc->in_bytes = proc->out_bytes = 0;
  proc->refcount = 1;

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

static void
fort_stat_flow_associate (PFORT_STAT stat, UINT64 flow_id,
                          UINT32 callout_id, UINT32 process_id)
{
  KIRQL irq;
  UINT64 flow_context;
  int proc_index;

  KeAcquireSpinLock(&stat->lock, &irq);

  proc_index = fort_stat_proc_index(stat, process_id);
  if (proc_index == -1) {
    proc_index = fort_stat_proc_add(stat, process_id);
    if (proc_index == -1)
      goto end;
  }

  flow_context = (UINT64) process_id | ((UINT64) proc_index << 32);

  fort_stat_proc_inc(stat, proc_index);

  if (!NT_SUCCESS(FwpsFlowAssociateContext0(flow_id,
      FWPS_LAYER_STREAM_V4, callout_id, flow_context))) {
    fort_stat_proc_dec(stat, proc_index);
  }

 end:
  KeReleaseSpinLock(&stat->lock, irq);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow +: %d\n", process_id);
}

static void
fort_stat_flow_delete (PFORT_STAT stat, UINT64 flow_context)
{
  KIRQL irq;
  const int proc_index = (int) (flow_context >> 32);

  KeAcquireSpinLock(&stat->lock, &irq);

  fort_stat_proc_dec(stat, proc_index);

  KeReleaseSpinLock(&stat->lock, irq);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow -: %d\n", (UINT32) flow_context);
}

static void
fort_stat_flow_classify (PFORT_STAT stat, UINT64 flow_context,
                         UINT32 data_len, BOOL inbound)
{
  PFORT_STAT_PROC proc;
  KIRQL irq;
  const UINT32 process_id = (UINT32) flow_context;
  const int proc_index = (int) (flow_context >> 32);

  KeAcquireSpinLock(&stat->lock, &irq);

  proc = &stat->procs[proc_index];

  if (inbound) {
    proc->in_bytes += data_len;
  } else {
    proc->out_bytes += data_len;
  }

  KeReleaseSpinLock(&stat->lock, irq);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow >: %d %d\n", (UINT32) flow_context, data_len);
}
