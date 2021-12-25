#ifndef FORTPCB_DRV_H
#define FORTPCB_DRV_H

#include "fortpcb_def.h"

#define FORT_DRIVER_MAJOR_FUNC_MAX (IRP_MJ_MAXIMUM_FUNCTION + 1)
static_assert(FORT_DRIVER_MAJOR_FUNC_MAX == 28, "Driver Major Functions Count Mismatch");

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_proxycb_drv_prepare(PDRIVER_DISPATCH *driver_major_funcs);
FORT_API void fort_proxycb_drv_setup(PDRIVER_DISPATCH *driver_major_funcs);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPCB_DRV_H
