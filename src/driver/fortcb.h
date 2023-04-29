#ifndef FORTCB_H
#define FORTCB_H

#include "fortdrv.h"

#include "proxycb/fortpcb_def.h"

enum {
    FORT_CALLBACK_SYSCB_POWER = 0,
    FORT_CALLBACK_SYSCB_TIME,
    FORT_CALLBACK_TIMER_CALLBACK,
    FORT_CALLBACK_WORKER_CALLBACK,
    FORT_CALLBACK_PSTREE_NOTIFY,
};

#if defined(__cplusplus)
extern "C" {
#endif

typedef void(WINAPI *FortCallbackFunc)(void);

FORT_API FortCallbackFunc fort_callback(int id, FortCallbackFunc func);

FORT_API void fort_callback_setup(PFORT_PROXYCB_INFO cb_info);

#ifdef __cplusplus
} // extern "C"
#endif

#define FORT_CALLBACK(id, T, func) (T) fort_callback((id), (FortCallbackFunc) (func))

#endif // FORTCB_H
