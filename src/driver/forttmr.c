/* Fort Firewall Timer */

typedef void (*FORT_TIMER_FUNC) (void);

typedef struct fort_timer {
  UINT32 running	: 1;
  UINT32 coalescable	: 1;
  UINT32 period		: 30;  /* milliseconds */

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
fort_timer_open (PFORT_TIMER timer, int period, BOOL coalescable,
                 FORT_TIMER_FUNC callback)
{
  timer->coalescable = coalescable;
  timer->period = period;
  timer->callback = callback;

  KeInitializeDpc(&timer->dpc, &fort_timer_callback, timer);
  KeInitializeTimer(&timer->id);
}

static void
fort_timer_close (PFORT_TIMER timer)
{
  if (!timer->running)
    return;

  timer->running = FALSE;

  KeCancelTimer(&timer->id);
  KeFlushQueuedDpcs();
}

static void
fort_timer_update (PFORT_TIMER timer, BOOL run)
{
  if ((BOOL) timer->running == run)
    return;

  timer->running = run;

  if (run) {
    const ULONG period = timer->period;
    const ULONG delay = timer->coalescable ? 500 : 0;
    LARGE_INTEGER due;
    due.QuadPart = period * -10000;  /* ms -> us */

    KeSetCoalescableTimer(&timer->id, due, period, delay, &timer->dpc);
  } else {
    KeCancelTimer(&timer->id);
  }
}
