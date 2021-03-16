/* Fort Firewall Driver Device */

#include "../version/fort_version.h"

#include "fortdev.h"

#include "common/fortdef.h"

#include "fortcout.h"

static PFORT_DEVICE g_device = NULL;

FORT_API PFORT_DEVICE fort_device()
{
    return g_device;
}

FORT_API void fort_device_set(PFORT_DEVICE device)
{
    g_device = device;
}

static void fort_worker_reauth(void)
{
    const FORT_CONF_FLAGS conf_flags = g_device->conf.conf_flags;
    NTSTATUS status;

    status = fort_callout_force_reauth(conf_flags, 0);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Worker Reauth: Error: %x\n",
                status);
    }
}

FORT_API void fort_app_period_timer(void)
{
    if (fort_conf_ref_period_update(&g_device->conf, FALSE, NULL)) {
        fort_worker_queue(&g_device->worker, FORT_WORKER_REAUTH, &fort_worker_reauth);
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

        fort_conf_zones_set(&g_device->conf, NULL);

        fort_callout_force_reauth(old_conf_flags, FORT_DEFER_FLUSH_ALL);
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

FORT_API NTSTATUS fort_device_control(PDEVICE_OBJECT device, PIRP irp)
{
    ULONG_PTR info = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UNUSED(device);

    const PIO_STACK_LOCATION irp_stack = IoGetCurrentIrpStackLocation(irp);
    const ULONG control_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;

    if (control_code != (ULONG) FORT_IOCTL_VALIDATE
            && !fort_device_flag(&g_device->conf, FORT_DEVICE_IS_VALIDATED))
        goto end;

    switch (control_code) {
    case FORT_IOCTL_VALIDATE: {
        const PFORT_CONF_VERSION conf_ver = irp->AssociatedIrp.SystemBuffer;
        const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

        if (len == sizeof(FORT_CONF_VERSION)) {
            if (conf_ver->driver_version == DRIVER_VERSION) {
                fort_device_flag_set(&g_device->conf, FORT_DEVICE_IS_VALIDATED, TRUE);
                status = STATUS_SUCCESS;
            }
        }
        break;
    }
    case FORT_IOCTL_SETCONF: {
        const PFORT_CONF_IO conf_io = irp->AssociatedIrp.SystemBuffer;
        const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

        if (len > sizeof(FORT_CONF_IO)) {
            const PFORT_CONF conf = &conf_io->conf;
            PFORT_CONF_REF conf_ref = fort_conf_ref_new(conf, len - FORT_CONF_IO_CONF_OFF);

            if (conf_ref == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                PFORT_STAT stat = &g_device->stat;

                const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(&g_device->conf, conf_ref);

                const UINT32 defer_flush_bits =
                        (stat->conf_group.limit_2bits ^ conf_io->conf_group.limit_2bits);

                fort_stat_conf_update(stat, conf_io);

                status = fort_callout_force_reauth(old_conf_flags, defer_flush_bits);
            }
        }
        break;
    }
    case FORT_IOCTL_SETFLAGS: {
        const PFORT_CONF_FLAGS conf_flags = irp->AssociatedIrp.SystemBuffer;
        const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

        if (len == sizeof(FORT_CONF_FLAGS)) {
            const FORT_CONF_FLAGS old_conf_flags =
                    fort_conf_ref_flags_set(&g_device->conf, conf_flags);

            const UINT32 defer_flush_bits =
                    (old_conf_flags.group_bits != conf_flags->group_bits ? FORT_DEFER_FLUSH_ALL
                                                                         : 0);

            status = fort_callout_force_reauth(old_conf_flags, defer_flush_bits);
        }
        break;
    }
    case FORT_IOCTL_GETLOG: {
        PVOID out = irp->AssociatedIrp.SystemBuffer;
        const ULONG out_len = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;

        if (out_len < FORT_BUFFER_SIZE) {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            status = fort_buffer_xmove(&g_device->buffer, irp, out, out_len, &info);

            if (status == STATUS_PENDING) {
                KIRQL cirq;

                IoMarkIrpPending(irp);

                IoAcquireCancelSpinLock(&cirq);
                IoSetCancelRoutine(irp, fort_device_cancel_pending);
                IoReleaseCancelSpinLock(cirq);

                return STATUS_PENDING;
            }
        }
        break;
    }
    case FORT_IOCTL_ADDAPP:
    case FORT_IOCTL_DELAPP: {
        const PFORT_APP_ENTRY app_entry = irp->AssociatedIrp.SystemBuffer;
        const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

        if (len > sizeof(FORT_APP_ENTRY) && len >= (sizeof(FORT_APP_ENTRY) + app_entry->path_len)) {
            PFORT_CONF_REF conf_ref = fort_conf_ref_take(&g_device->conf);

            if (conf_ref == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                if (control_code == (ULONG) FORT_IOCTL_ADDAPP) {
                    status = fort_conf_ref_exe_add_entry(conf_ref, app_entry, FALSE);
                } else {
                    fort_conf_ref_exe_del_entry(conf_ref, app_entry);
                    status = STATUS_SUCCESS;
                }

                fort_conf_ref_put(&g_device->conf, conf_ref);

                if (NT_SUCCESS(status)) {
                    fort_worker_reauth();
                }
            }
        }
        break;
    }
    case FORT_IOCTL_SETZONES: {
        const PFORT_CONF_ZONES zones = irp->AssociatedIrp.SystemBuffer;
        const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

        if (len >= FORT_CONF_ZONES_DATA_OFF) {
            PFORT_CONF_ZONES conf_zones = fort_conf_zones_new(zones, len);

            if (conf_zones == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                fort_conf_zones_set(&g_device->conf, conf_zones);

                fort_worker_reauth();

                status = STATUS_SUCCESS;
            }
        }
        break;
    }
    case FORT_IOCTL_SETZONEFLAG: {
        const PFORT_CONF_ZONE_FLAG zone_flag = irp->AssociatedIrp.SystemBuffer;
        const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

        if (len == sizeof(FORT_CONF_ZONE_FLAG)) {
            fort_conf_zone_flag_set(&g_device->conf, zone_flag);

            fort_worker_reauth();

            status = STATUS_SUCCESS;
        }
        break;
    }
    default:
        break;
    }

end:
    if (!NT_SUCCESS(status) && status != FORT_STATUS_USER_ERROR) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Device Control: Error: %x\n",
                status);
    }

    fort_request_complete_info(irp, status, info);

    return status;
}
