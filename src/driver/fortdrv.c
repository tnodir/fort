/* Fort Firewall Driver */

#include "fortdrv.h"

#include "common/fortioctl.h"

#include "fortcb.h"
#include "fortdev.h"
#include "forttrace.h"
#include "fortutl.h"

static void fort_driver_delete_device(PDRIVER_OBJECT driver)
{
    IoDeleteDevice(driver->DeviceObject);
    driver->DeviceObject = NULL;
}

static void fort_driver_unload(PDRIVER_OBJECT driver)
{
    fort_device_unload();

    if (driver->DeviceObject != NULL) {
        /* Unregister Device Callbacks */
        IoUnregisterShutdownNotification(driver->DeviceObject);

        /* Delete Device Link */
        UNICODE_STRING device_link;
        RtlInitUnicodeString(&device_link, FORT_DOS_DEVICE_NAME);
        IoDeleteSymbolicLink(&device_link);

        fort_driver_delete_device(driver);
    }
}

static NTSTATUS fort_driver_load(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    NTSTATUS status;

    /* Use NX Non-Paged Pool */
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    status = fort_system32_path_init(driver, reg_path);
    if (!NT_SUCCESS(status)) {
        LOG("Driver Path Init: Error: %x\n", status);
        return status;
    }

    UNICODE_STRING device_name;
    RtlInitUnicodeString(&device_name, FORT_NT_DEVICE_NAME);

    /* TODO: Use IoCreateDeviceSecure() with custom SDDL for Service SID */
    PDEVICE_OBJECT device_obj;
    status = IoCreateDevice(driver, sizeof(FORT_DEVICE), &device_name, FORT_DEVICE_TYPE, 0,
            /*exclusive=*/TRUE, &device_obj);
    if (!NT_SUCCESS(status)) {
        LOG("Create Device: Error: %x\n", status);
        return status;
    }

    device_obj->Flags |= DO_BUFFERED_IO;

    status = IoRegisterShutdownNotification(device_obj);
    if (!NT_SUCCESS(status)) {
        LOG("Register Shutdown: Error: %x\n", status);
        fort_driver_delete_device(driver);
        return status;
    }

    UNICODE_STRING device_link;

    RtlInitUnicodeString(&device_link, FORT_DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&device_link, &device_name);
    if (!NT_SUCCESS(status)) {
        LOG("Create Link: Error: %x\n", status);
        return status;
    }

    driver->DriverUnload = &fort_driver_unload;
    driver->MajorFunction[IRP_MJ_CREATE] = &fort_device_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = &fort_device_close;
    driver->MajorFunction[IRP_MJ_CLEANUP] = &fort_device_cleanup;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &fort_device_control;
    driver->MajorFunction[IRP_MJ_SHUTDOWN] = &fort_device_shutdown;

    return fort_device_load(device_obj);
}

NTSTATUS DriverCallbacksSetup(PFORT_PROXYCB_INFO cb_info)
{
    fort_callback_setup(cb_info);

    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    const NTSTATUS status = fort_driver_load(driver, reg_path);

    if (!NT_SUCCESS(status)) {
        LOG("Entry: Error: %x\n", status);
        TRACE(FORT_DRIVER_ENTRY_ERROR, status, 0, 0);

        fort_driver_unload(driver);
    }

    return status;
}

#define FORT_STACK_USAGE_FREE_MIN (1 * 1024)

FORT_API void fort_check_stack_usage(const char *func_name)
{
    const UINT32 free_size = (UINT32) IoGetRemainingStackSize();

    if (free_size < FORT_STACK_USAGE_FREE_MIN) {
        LOG("Stack Overflow: %s: remaining=%d\n", func_name, free_size);
    }
}
