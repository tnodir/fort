#ifndef FORTPCB_DRV_H
#define FORTPCB_DRV_H

#include "../fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_proxycb_drv_setup(PDRIVER_DISPATCH *driver_major_funcs);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPCB_DRV_H
