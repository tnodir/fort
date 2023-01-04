/* Fort Firewall Driver Device */

#include "fortdev.h"

#include "common/fortioctl.h"
#include "common/fortprov.h"

#include "fortcout.h"
#include "fortscb.h"
#include "forttrace.h"

static PFORT_DEVICE g_device = NULL;

FORT_API PFORT_DEVICE fort_device()
{
    return g_device;
}

static void fort_device_set(PFORT_DEVICE device)
{
    g_device = device;
}

static void fort_device_init(PDEVICE_OBJECT device)
{
    fort_device_set(device->DeviceExtension);

    RtlZeroMemory(g_device, sizeof(FORT_DEVICE));

    g_device->object = device;
}

static void NTAPI fort_worker_reauth(void)
{
    const FORT_CONF_FLAGS conf_flags = g_device->conf.conf_flags;
    NTSTATUS status;

    status = fort_callout_force_reauth(conf_flags);

    if (!NT_SUCCESS(status)) {
        LOG("Worker Reauth: Error: %x\n", status);
        TRACE(FORT_DEVICE_WORKER_REAUTH_ERROR, status, 0, 0);
    }
}

FORT_API void NTAPI fort_app_period_timer(void)
{
    if (fort_conf_ref_period_update(&g_device->conf, /*force=*/FALSE, /*periods_n=*/NULL)) {
        fort_worker_queue(&g_device->worker, FORT_WORKER_REAUTH);
    }
}

FORT_API NTSTATUS fort_device_create(PDEVICE_OBJECT device, PIRP irp)
{
    NTSTATUS status = STATUS_SUCCESS;

    UNUSED(device);

    /* Device opened */
    if (fort_device_flag_set(&g_device->conf, FORT_DEVICE_IS_OPENED, TRUE)
            & FORT_DEVICE_IS_OPENED) {
        status = STATUS_SHARING_VIOLATION; /* Only one client may connect */
    }

    if (NT_SUCCESS(status)) {
        /* Clear buffer */
        fort_buffer_clear(&g_device->buffer);
    }

    fort_request_complete(irp, status);

    return status;
}

FORT_API NTSTATUS fort_device_close(PDEVICE_OBJECT device, PIRP irp)
{
    UNUSED(device);

    fort_request_complete(irp, STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS fort_device_cleanup(PDEVICE_OBJECT device, PIRP irp)
{
    UNUSED(device);

    /* Device closed */
    fort_device_flag_set(
            &g_device->conf, (FORT_DEVICE_IS_OPENED | FORT_DEVICE_IS_VALIDATED), FALSE);

    /* Clear conf */
    {
        const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(&g_device->conf, NULL);
        FORT_CONF_FLAGS conf_flags = g_device->conf.conf_flags;

        fort_conf_zones_set(&g_device->conf, NULL);

        fort_stat_conf_flags_update(&g_device->stat, &conf_flags);

        fort_callout_force_reauth(old_conf_flags);
    }

    /* Clear buffer */
    fort_buffer_clear(&g_device->buffer);

    fort_request_complete(irp, STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

static void fort_device_cancel_pending(PDEVICE_OBJECT device, PIRP irp)
{
    ULONG_PTR info;
    NTSTATUS status;

    UNUSED(device);

    status = fort_buffer_cancel_pending(&g_device->buffer, irp, &info);

    IoReleaseCancelSpinLock(irp->CancelIrql); /* before IoCompleteRequest()! */

    fort_request_complete_info(irp, status, info);
}

static NTSTATUS fort_device_control_validate(const PFORT_CONF_VERSION conf_ver, ULONG len)
{
    if (len == sizeof(FORT_CONF_VERSION)) {
        if (conf_ver->driver_version == DRIVER_VERSION) {
            fort_device_flag_set(&g_device->conf, FORT_DEVICE_IS_VALIDATED, TRUE);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_setconf(const PFORT_CONF_IO conf_io, ULONG len)
{
    if (len > sizeof(FORT_CONF_IO)) {
        const PFORT_CONF conf = &conf_io->conf;
        PFORT_CONF_REF conf_ref = fort_conf_ref_new(conf, len - FORT_CONF_IO_CONF_OFF);

        if (conf_ref == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        } else {
            const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(&g_device->conf, conf_ref);

            fort_shaper_conf_update(&g_device->shaper, conf_io);
            fort_stat_conf_update(&g_device->stat, conf_io);

            return fort_callout_force_reauth(old_conf_flags);
        }
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_setflags(const PFORT_CONF_FLAGS conf_flags, ULONG len)
{
    if (len == sizeof(FORT_CONF_FLAGS)) {
        const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_flags_set(&g_device->conf, conf_flags);

        fort_shaper_conf_flags_update(&g_device->shaper, conf_flags);
        fort_stat_conf_flags_update(&g_device->stat, conf_flags);

        return fort_callout_force_reauth(old_conf_flags);
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_getlog(PVOID out, ULONG out_len, PIRP irp, ULONG_PTR *info)
{
    if (out_len < FORT_BUFFER_SIZE) {
        return STATUS_BUFFER_TOO_SMALL;
    } else {
        const NTSTATUS status = fort_buffer_xmove(&g_device->buffer, irp, out, out_len, info);

        if (status == STATUS_PENDING) {
            KIRQL cirq;

            IoMarkIrpPending(irp);

            IoAcquireCancelSpinLock(&cirq);
            IoSetCancelRoutine(irp, fort_device_cancel_pending);
            IoReleaseCancelSpinLock(cirq);
        }
        return status;
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_app(const PFORT_APP_ENTRY app_entry, ULONG len, BOOL is_adding)
{
    if (len > sizeof(FORT_APP_ENTRY) && len >= (sizeof(FORT_APP_ENTRY) + app_entry->path_len)) {
        PFORT_CONF_REF conf_ref = fort_conf_ref_take(&g_device->conf);

        if (conf_ref == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        } else {
            NTSTATUS status;

            if (is_adding) {
                status = fort_conf_ref_exe_add_entry(conf_ref, app_entry, FALSE);
            } else {
                fort_conf_ref_exe_del_entry(conf_ref, app_entry);
                status = STATUS_SUCCESS;
            }

            fort_conf_ref_put(&g_device->conf, conf_ref);

            if (NT_SUCCESS(status)) {
                fort_worker_reauth();
            }

            return status;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_setzones(const PFORT_CONF_ZONES zones, ULONG len)
{
    if (len >= FORT_CONF_ZONES_DATA_OFF) {
        PFORT_CONF_ZONES conf_zones = fort_conf_zones_new(zones, len);

        if (conf_zones == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        } else {
            fort_conf_zones_set(&g_device->conf, conf_zones);

            fort_worker_reauth();

            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_setzoneflag(const PFORT_CONF_ZONE_FLAG zone_flag, ULONG len)
{
    if (len == sizeof(FORT_CONF_ZONE_FLAG)) {
        fort_conf_zone_flag_set(&g_device->conf, zone_flag);

        fort_worker_reauth();

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_process(
        const PIO_STACK_LOCATION irp_stack, PIRP irp, ULONG_PTR *info)
{
    const int control_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;

    if (control_code != FORT_IOCTL_VALIDATE
            && fort_device_flag(&g_device->conf, FORT_DEVICE_IS_VALIDATED) == 0)
        return STATUS_INVALID_PARAMETER;

    PVOID buffer = irp->AssociatedIrp.SystemBuffer;
    const ULONG in_len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;
    const ULONG out_len = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (control_code) {
    case FORT_IOCTL_VALIDATE:
        return fort_device_control_validate(buffer, in_len);
    case FORT_IOCTL_SETCONF:
        return fort_device_control_setconf(buffer, in_len);
    case FORT_IOCTL_SETFLAGS:
        return fort_device_control_setflags(buffer, in_len);
    case FORT_IOCTL_GETLOG:
        return fort_device_control_getlog(buffer, out_len, irp, info);
    case FORT_IOCTL_ADDAPP:
    case FORT_IOCTL_DELAPP:
        return fort_device_control_app(buffer, in_len, (control_code == FORT_IOCTL_ADDAPP));
    case FORT_IOCTL_SETZONES:
        return fort_device_control_setzones(buffer, in_len);
    case FORT_IOCTL_SETZONEFLAG:
        return fort_device_control_setzoneflag(buffer, in_len);
    default:
        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

FORT_API NTSTATUS fort_device_control(PDEVICE_OBJECT device, PIRP irp)
{
    ULONG_PTR info = 0;

    UNUSED(device);

    const PIO_STACK_LOCATION irp_stack = IoGetCurrentIrpStackLocation(irp);
    const NTSTATUS status = fort_device_control_process(irp_stack, irp, &info);

    if (!NT_SUCCESS(status) && status != FORT_STATUS_USER_ERROR) {
        LOG("Device Control: Error: %x\n", status);
        TRACE(FORT_DEVICE_DEVICE_CONTROL_ERROR, status, 0, 0);
    }

    if (status != STATUS_PENDING) {
        fort_request_complete_info(irp, status, info);
    }

    return status;
}

FORT_API NTSTATUS fort_device_load(PDEVICE_OBJECT device)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    fort_device_init(device);

    fort_worker_func_set(&fort_device()->worker, FORT_WORKER_REAUTH, &fort_worker_reauth);
    fort_worker_func_set(&fort_device()->worker, FORT_WORKER_PSTREE, &fort_pstree_enum_processes);

    fort_device_conf_open(&fort_device()->conf);
    fort_buffer_open(&fort_device()->buffer);
    fort_stat_open(&fort_device()->stat);
    fort_shaper_open(&fort_device()->shaper);
    fort_timer_open(&fort_device()->log_timer, 500, /*oneshot=*/FALSE, /*coalescable=*/FALSE,
            &fort_callout_timer);
    fort_timer_open(&fort_device()->app_timer, 60000, /*oneshot=*/FALSE, /*coalescable=*/TRUE,
            &fort_app_period_timer);
    fort_pstree_open(&fort_device()->ps_tree);

    /* Unregister old filters provider */
    {
        fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_PROV_BOOT, fort_prov_is_boot());

        fort_prov_unregister(NULL);
    }

    /* Install callouts */
    status = fort_callout_install(device);

    /* Register worker */
    if (NT_SUCCESS(status)) {
        status = fort_worker_register(device, &fort_device()->worker);
    }

    /* Register filters provider */
    if (NT_SUCCESS(status)) {
        const BOOL prov_boot = fort_device_flag(&fort_device()->conf, FORT_DEVICE_PROV_BOOT) != 0;

        status = fort_prov_register(0, prov_boot);
    }

    /* Register power state change callback */
    if (NT_SUCCESS(status)) {
        status = fort_syscb_power_register();
    }

    /* Register system time change callback */
    if (NT_SUCCESS(status)) {
        status = fort_syscb_time_register();
    }

    /* Enumerate processes in background */
    if (NT_SUCCESS(status)) {
        fort_worker_queue(&fort_device()->worker, FORT_WORKER_PSTREE);
    }

    return status;
}

FORT_API void fort_device_unload()
{
    if (fort_device() == NULL)
        return;

    /* Stop system notifiers */
    fort_syscb_power_unregister();
    fort_syscb_time_unregister();

    /* Stop timers */
    fort_timer_close(&fort_device()->app_timer);
    fort_timer_close(&fort_device()->log_timer);

    /* Stop worker threads */
    fort_worker_unregister(&fort_device()->worker);

    /* Stop process monitor */
    fort_pstree_close(&fort_device()->ps_tree);

    /* Stop packets shaper */
    fort_shaper_close(&fort_device()->shaper);

    /* Stop stat & buffer controllers */
    fort_stat_close(&fort_device()->stat);
    fort_buffer_close(&fort_device()->buffer);

    /* Uninstall callouts */
    fort_callout_remove();

    /* Unregister filters provider */
    if (fort_device_flag(&fort_device()->conf, FORT_DEVICE_PROV_BOOT) == 0) {
        fort_prov_unregister(NULL);
    }

    fort_device_set(NULL);
}
