/* Fort Firewall Driver */

#include "fortdrv.h"

#include "common/fortdef.h"
#include "common/fortprov.h"

#include "fortdev.h"
#include "fortcout.h"

static NTSTATUS fort_callback_register(
        PCWSTR sourcePath, PCALLBACK_OBJECT *cb_obj, PVOID *cb_reg, PCALLBACK_FUNCTION cb_func)
{
    OBJECT_ATTRIBUTES obj_attr;
    UNICODE_STRING obj_name;
    NTSTATUS status;

    RtlInitUnicodeString(&obj_name, sourcePath);

    InitializeObjectAttributes(&obj_attr, &obj_name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ExCreateCallback(cb_obj, &obj_attr, FALSE, TRUE);

    if (NT_SUCCESS(status)) {
        *cb_reg = ExRegisterCallback(*cb_obj, cb_func, NULL);
    }

    return status;
}

static void fort_callback_unregister(PCALLBACK_OBJECT cb_obj, PVOID cb_reg)
{
    if (cb_reg != NULL) {
        ExUnregisterCallback(cb_reg);
    }

    if (cb_obj != NULL) {
        ObDereferenceObject(cb_obj);
    }
}

static void fort_power_callback(PVOID context, PVOID event, PVOID specifics)
{
    UNUSED(context);

    if (event != (PVOID) PO_CB_SYSTEM_STATE_LOCK)
        return;

    const BOOL power_off = (specifics == NULL);

    fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_POWER_OFF, power_off);

    if (power_off) {
        fort_callout_defer_flush();
    }
}

static NTSTATUS fort_power_callback_register(void)
{
    return fort_callback_register(L"\\Callback\\PowerState", &fort_device()->power_cb_obj,
            &fort_device()->power_cb_reg, fort_power_callback);
}

static void fort_power_callback_unregister(void)
{
    fort_callback_unregister(fort_device()->power_cb_obj, fort_device()->power_cb_reg);
}

static void fort_systime_callback(PVOID context, PVOID event, PVOID specifics)
{
    UNUSED(context);
    UNUSED(event);
    UNUSED(specifics);

    if (fort_device()->app_timer.running) {
        fort_app_period_timer();
    }
}

static NTSTATUS fort_systime_callback_register(void)
{
    return fort_callback_register(L"\\Callback\\SetSystemTime", &fort_device()->systime_cb_obj,
            &fort_device()->systime_cb_reg, fort_systime_callback);
}

static void fort_systime_callback_unregister(void)
{
    fort_callback_unregister(fort_device()->systime_cb_obj, fort_device()->systime_cb_reg);
}

static void fort_driver_unload(PDRIVER_OBJECT driver)
{
    if (fort_device() != NULL) {
        fort_callout_defer_flush();

        fort_timer_close(&fort_device()->app_timer);
        fort_timer_close(&fort_device()->log_timer);
        fort_defer_close(&fort_device()->defer);
        fort_stat_close(&fort_device()->stat);
        fort_buffer_close(&fort_device()->buffer);

        fort_worker_unregister(&fort_device()->worker);

        fort_power_callback_unregister();
        fort_systime_callback_unregister();

        if (!fort_device_flag(&fort_device()->conf, FORT_DEVICE_PROV_BOOT)) {
            fort_prov_unregister(0);
        }

        fort_callout_remove();
    }

    /* Delete Device Link */
    {
        UNICODE_STRING device_link;

        RtlInitUnicodeString(&device_link, FORT_DOS_DEVICE_NAME);
        IoDeleteSymbolicLink(&device_link);
    }

    IoDeleteDevice(driver->DeviceObject);
}

static NTSTATUS fort_bfe_wait(void)
{
    LARGE_INTEGER delay;
    delay.QuadPart = -5000000; /* sleep 500000us (500ms) */
    int count = 600; /* wait for 5 minutes */

    do {
        const FWPM_SERVICE_STATE state = FwpmBfeStateGet0();
        if (state == FWPM_SERVICE_RUNNING)
            return STATUS_SUCCESS;

        KeDelayExecutionThread(KernelMode, FALSE, &delay);
    } while (--count >= 0);

    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    UNICODE_STRING device_name;
    PDEVICE_OBJECT device;
    NTSTATUS status;

    UNUSED(reg_path);

    // Use NX Non-Paged Pool
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    /* Wait for BFE to start */
    status = fort_bfe_wait();
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                "FORT: Entry: Error: BFE is not running\n");
        return status;
    }

    RtlInitUnicodeString(&device_name, FORT_NT_DEVICE_NAME);
    status = IoCreateDevice(
            driver, sizeof(FORT_DEVICE), &device_name, FORT_DEVICE_TYPE, 0, FALSE, &device);

    if (NT_SUCCESS(status)) {
        UNICODE_STRING device_link;

        RtlInitUnicodeString(&device_link, FORT_DOS_DEVICE_NAME);
        status = IoCreateSymbolicLink(&device_link, &device_name);

        if (NT_SUCCESS(status)) {
            driver->MajorFunction[IRP_MJ_CREATE] = fort_device_create;
            driver->MajorFunction[IRP_MJ_CLOSE] = fort_device_close;
            driver->MajorFunction[IRP_MJ_CLEANUP] = fort_device_cleanup;
            driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = fort_device_control;
            driver->DriverUnload = fort_driver_unload;

            device->Flags |= DO_BUFFERED_IO;

            fort_device_set(device->DeviceExtension);

            RtlZeroMemory(fort_device(), sizeof(FORT_DEVICE));

            fort_device_conf_open(&fort_device()->conf);
            fort_buffer_open(&fort_device()->buffer);
            fort_stat_open(&fort_device()->stat);
            fort_defer_open(&fort_device()->defer);
            fort_timer_open(&fort_device()->log_timer, 500, FALSE, &fort_callout_timer);
            fort_timer_open(&fort_device()->app_timer, 60000, TRUE, &fort_app_period_timer);

            /* Unregister old filters provider */
            {
                fort_device_flag_set(
                        &fort_device()->conf, FORT_DEVICE_PROV_BOOT, fort_prov_is_boot());

                fort_prov_unregister(0);
            }

            /* Install callouts */
            status = fort_callout_install(device);

            /* Register worker */
            if (NT_SUCCESS(status)) {
                status = fort_worker_register(device, &fort_device()->worker);
            }

            /* Register filters provider */
            if (NT_SUCCESS(status)) {
                const BOOL prov_boot =
                        fort_device_flag(&fort_device()->conf, FORT_DEVICE_PROV_BOOT);

                status = fort_prov_register(0, prov_boot);
            }

            /* Register power state change callback */
            if (NT_SUCCESS(status)) {
                status = fort_power_callback_register();
            }

            /* Register system time change callback */
            if (NT_SUCCESS(status)) {
                status = fort_systime_callback_register();
            }
        }
    }

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Entry: Error: %x\n", status);
        fort_driver_unload(driver);
    }

    return status;
}
