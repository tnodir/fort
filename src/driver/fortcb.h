#ifndef FORTCB_H
#define FORTCB_H

#include "fortdrv.h"

enum {
    FORT_SYSCB_POWER = 0,
    FORT_SYSCB_TIME,
    FORT_TIMER_CALLBACK,
    FORT_WORKER_CALLBACK,
};

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*FortCallbackFunc)(void);

FORT_API FortCallbackFunc fort_callback(int id, FortCallbackFunc func);

#ifdef __cplusplus
} // extern "C"
#endif

#define FORT_CALLBACK(id, T, func) (T) fort_callback((id), (FortCallbackFunc) (func))

#endif // FORTCB_H
