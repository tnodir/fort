#ifndef FORTDEV_H
#define FORTDEV_H

#include "fortdrv.h"

#include "fortbuf.h"
#include "fortcnf.h"
#include "fortpkt.h"
#include "fortps.h"
#include "fortstat.h"
#include "forttmr.h"
#include "fortwrk.h"

typedef struct fort_device
{
    PDEVICE_OBJECT device;

    EX_RUNDOWN_REF reauth_rundown;

    PCALLBACK_OBJECT power_cb_obj;
    PVOID power_cb_reg;

    PCALLBACK_OBJECT systime_cb_obj;
    PVOID systime_cb_reg;

    FORT_DEVICE_CONF conf;
    FORT_BUFFER buffer;
    FORT_STAT stat;
    FORT_PENDING pending;
    FORT_SHAPER shaper;
    FORT_PSTREE ps_tree;
    FORT_TIMER log_timer;
    FORT_TIMER app_timer;
    FORT_WORKER worker;
} FORT_DEVICE, *PFORT_DEVICE;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API PFORT_DEVICE fort_device(void);

FORT_API void fort_device_set(PFORT_DEVICE device);

FORT_API void fort_device_on_system_time(void);

FORT_API NTSTATUS fort_device_create(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_close(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_cleanup(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_control(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_shutdown(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_load(PVOID device_param);

FORT_API void fort_device_unload(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTDEV_H
