#ifndef FORTCB_H
#define FORTCB_H

#include "fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS fort_callback_power_register(void);

FORT_API void fort_callback_power_unregister(void);

FORT_API NTSTATUS fort_callback_systime_register(void);

FORT_API void fort_callback_systime_unregister(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCB_H
