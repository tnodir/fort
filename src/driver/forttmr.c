/* Fort Firewall Timer */

#include "forttmr.h"

#include "fortcb.h"

static void NTAPI fort_timer_callback(PKDPC dpc, PFORT_TIMER timer, PVOID arg1, PVOID arg2)
{
    UNUSED(dpc);
    UNUSED(arg1);
    UNUSED(arg2);

    if (timer->callback != NULL) {
        timer->callback();
    }
}

FORT_API void fort_timer_open(
        PFORT_TIMER timer, int period, BOOL coalescable, FORT_TIMER_FUNC callback)
{
    timer->coalescable = coalescable;
    timer->period = period;
    timer->callback = callback;

    KeInitializeDpc(&timer->dpc,
            FORT_CALLBACK(FORT_TIMER_CALLBACK, PKDEFERRED_ROUTINE, fort_timer_callback), timer);
    KeInitializeTimer(&timer->id);
}

FORT_API void fort_timer_close(PFORT_TIMER timer)
{
    if (!timer->running)
        return;

    timer->running = FALSE;

    KeCancelTimer(&timer->id);
    KeFlushQueuedDpcs();
}

FORT_API void fort_timer_update(PFORT_TIMER timer, BOOL run)
{
    if ((BOOL) timer->running == run)
        return;

    timer->running = run;

    if (run) {
        const ULONG period = timer->period;
        const ULONG delay = timer->coalescable ? 500 : 0;

        LARGE_INTEGER due;
        due.QuadPart = period * -10000; /* ms -> us */

        KeSetCoalescableTimer(&timer->id, due, period, delay, &timer->dpc);
    } else {
        KeCancelTimer(&timer->id);
    }
}
