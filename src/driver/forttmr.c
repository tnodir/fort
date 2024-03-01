/* Fort Firewall Timer */

#include "forttmr.h"

#include "fortcb.h"
#include "fortdbg.h"

static UCHAR fort_timer_flags_set(PFORT_TIMER timer, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&timer->flags, flags) : InterlockedAnd8(&timer->flags, ~flags);
}

static UCHAR fort_timer_flags(PFORT_TIMER timer)
{
    return fort_timer_flags_set(timer, 0, TRUE);
}

static void NTAPI fort_timer_callback(PKDPC dpc, PFORT_TIMER timer, PVOID arg1, PVOID arg2)
{
    UNUSED(dpc);
    UNUSED(arg1);
    UNUSED(arg2);

    FORT_CHECK_STACK(FORT_TIMER_CALLBACK);

    const UCHAR flags = fort_timer_flags(timer);
    if ((flags & FORT_TIMER_ONESHOT) != 0) {
        fort_timer_flags_set(timer, FORT_TIMER_RUNNING, FALSE);
    }

    if (timer->callback != NULL) {
        timer->callback();
    }
}

FORT_API void fort_timer_open(
        PFORT_TIMER timer, ULONG period, UCHAR flags, FORT_TIMER_FUNC callback)
{
    timer->period = period;
    timer->callback = callback;
    timer->flags = flags;

    KeInitializeDpc(&timer->dpc,
            FORT_CALLBACK(FORT_CALLBACK_TIMER_CALLBACK, PKDEFERRED_ROUTINE, &fort_timer_callback),
            timer);
    KeInitializeTimer(&timer->id);
}

FORT_API void fort_timer_close(PFORT_TIMER timer)
{
    const UCHAR old_flags = fort_timer_flags_set(timer, FORT_TIMER_RUNNING, FALSE);
    if ((old_flags & FORT_TIMER_RUNNING) == 0)
        return;

    KeCancelTimer(&timer->id);
    KeFlushQueuedDpcs();
}

FORT_API BOOL fort_timer_is_running(PFORT_TIMER timer)
{
    const UCHAR flags = fort_timer_flags(timer);
    return (flags & FORT_TIMER_RUNNING) != 0;
}

void fort_timer_set_running(PFORT_TIMER timer, BOOL run)
{
    const UCHAR flags = fort_timer_flags_set(timer, FORT_TIMER_RUNNING, run);

    const BOOL was_run = (flags & FORT_TIMER_RUNNING) != 0;
    if (run == was_run)
        return;

    if (run) {
        const ULONG period = timer->period;
        const ULONG interval = (flags & FORT_TIMER_ONESHOT) != 0 ? 0 : period;
        const ULONG delay = (flags & FORT_TIMER_COALESCABLE) != 0 ? 500 : 0;

        const LARGE_INTEGER due = {
            .QuadPart = (INT64) period * -10000LL /* ms -> us */
        };

        KeSetCoalescableTimer(&timer->id, due, interval, delay, &timer->dpc);
    } else {
        KeCancelTimer(&timer->id);
    }
}
