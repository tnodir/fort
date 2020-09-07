#ifndef FORTTMR_H
#define FORTTMR_H

#include "fortdrv.h"

typedef void (*FORT_TIMER_FUNC)(void);

typedef struct fort_timer
{
    UINT32 running : 1;
    UINT32 coalescable : 1;
    UINT32 period : 30; /* milliseconds */

    FORT_TIMER_FUNC callback;

    KDPC dpc;
    KTIMER id;
} FORT_TIMER, *PFORT_TIMER;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_timer_open(
        PFORT_TIMER timer, int period, BOOL coalescable, FORT_TIMER_FUNC callback);

FORT_API void fort_timer_close(PFORT_TIMER timer);

FORT_API void fort_timer_update(PFORT_TIMER timer, BOOL run);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTTMR_H
