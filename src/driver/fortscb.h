#ifndef FORTSCB_H
#define FORTSCB_H

#include "fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS fort_syscb_power_register(void);

FORT_API void fort_syscb_power_unregister(void);

FORT_API NTSTATUS fort_syscb_time_register(void);

FORT_API void fort_syscb_time_unregister(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTSCB_H
