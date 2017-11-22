/* Fort Firewall Timer */

typedef void (*FORT_TIMER_FUNC) (void);

typedef struct fort_timer {
  UINT32 running	: 1;

  FORT_TIMER_FUNC callback;

  KDPC dpc;
  KTIMER id;
} FORT_TIMER, *PFORT_TIMER;


static void
fort_timer_callback (PKDPC dpc, PFORT_TIMER timer, PVOID arg1, PVOID arg2)
{
  UNUSED(dpc);
  UNUSED(arg1);
  UNUSED(arg2);

  if (timer->callback != NULL) {
    timer->callback();
  }
}

static void
fort_timer_init (PFORT_TIMER timer, FORT_TIMER_FUNC callback)
{
  timer->callback = callback;

  KeInitializeDpc(&timer->dpc, &fort_timer_callback, timer);
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
