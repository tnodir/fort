#ifndef FORTTMR_H
#define FORTTMR_H

#include "fortdrv.h"

typedef void (*FORT_TIMER_FUNC)(void);

#define FORT_TIMER_RUNNING     0x01
#define FORT_TIMER_ONESHOT     0x02
#define FORT_TIMER_COALESCABLE 0x04

typedef struct fort_timer
{
    UCHAR volatile flags;

    ULONG period; /* milliseconds */

    FORT_TIMER_FUNC callback;

    KDPC dpc;
    KTIMER id;
} FORT_TIMER, *PFORT_TIMER;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_timer_open(
        PFORT_TIMER timer, ULONG period, UCHAR flags, FORT_TIMER_FUNC callback);

FORT_API void fort_timer_close(PFORT_TIMER timer);

FORT_API BOOL fort_timer_is_running(PFORT_TIMER timer);

FORT_API void fort_timer_set_running(PFORT_TIMER timer, BOOL run);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTTMR_H
