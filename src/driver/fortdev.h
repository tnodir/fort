#ifndef FORTDEV_H
#define FORTDEV_H

#include "fortdrv.h"

#include "fortbuf.h"
#include "fortcnf.h"
#include "fortpkt.h"
#include "fortstat.h"
#include "forttmr.h"
#include "fortwrk.h"

typedef struct fort_device
{
    UINT32 connect4_id;
    UINT32 accept4_id;

    PCALLBACK_OBJECT power_cb_obj;
    PVOID power_cb_reg;

    PCALLBACK_OBJECT systime_cb_obj;
    PVOID systime_cb_reg;

    FORT_DEVICE_CONF conf;
    FORT_BUFFER buffer;
    FORT_STAT stat;
    FORT_DEFER defer;
    FORT_TIMER log_timer;
    FORT_TIMER app_timer;
    FORT_WORKER worker;
} FORT_DEVICE, *PFORT_DEVICE;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API PFORT_DEVICE fort_device();

FORT_API void NTAPI fort_app_period_timer(void);

FORT_API NTSTATUS fort_device_create(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_close(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_cleanup(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_control(PDEVICE_OBJECT device, PIRP irp);

FORT_API NTSTATUS fort_device_load(PDEVICE_OBJECT device);

FORT_API void fort_device_unload();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTDEV_H
