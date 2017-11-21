/* Fort Firewall Timer */

typedef struct fort_timer {
  UINT32 running	: 1;

  KDPC dpc;
  KTIMER id;
} FORT_TIMER, *PFORT_TIMER;


static void
fort_timer_callback (PKDPC dpc, PFORT_BUFFER buf, PVOID arg1, PVOID arg2)
{
  PIRP irp = NULL;
  ULONG info = 0;

  UNUSED(dpc);
  UNUSED(arg1);
  UNUSED(arg2);

  KeAcquireSpinLockAtDpcLevel(&buf->lock);
  if (buf->irp != NULL && buf->out_top) {
    irp = buf->irp;
    buf->irp = NULL;

    info = buf->out_top;
    buf->out_top = 0;
  }
  KeReleaseSpinLockFromDpcLevel(&buf->lock);

  if (irp != NULL) {
    fort_request_complete_info(irp, STATUS_SUCCESS, info);
  }
}

static void
fort_timer_init (PFORT_TIMER timer, PFORT_BUFFER buf)
{
  KeInitializeDpc(&timer->dpc, &fort_timer_callback, buf);
  KeInitializeTimer(&timer->id);
}

static void
fort_timer_close (PFORT_TIMER timer)
{
  timer->running = FALSE;

  KeCancelTimer(&timer->id);
  KeFlushQueuedDpcs();
}

static void
fort_timer_update (PFORT_TIMER timer, const FORT_CONF_FLAGS conf_flags)
{
  const BOOL run = conf_flags.log_blocked || conf_flags.log_stat;

  if (timer->running == run)
    return;

  timer->running = run;

  if (run) {
    const LONG period = 500;  // 500ms
    LARGE_INTEGER due;
    due.QuadPart = period * -10000;  // 500000us

    KeSetTimerEx(&timer->id, due, period, &timer->dpc);
  } else {
    KeCancelTimer(&timer->id);
  }
}
