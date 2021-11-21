/* Fort Firewall Driver */

#include "fortdrv.h"

#include "common/fortdef.h"

#include "fortdev.h"

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

    return STATUS_FWP_TCPIP_NOT_READY;
}

static void fort_driver_unload(PDRIVER_OBJECT driver)
{
    fort_device_unload();

    /* Delete Device Link */
    if (driver->DeviceObject != NULL) {
        UNICODE_STRING device_link;

        RtlInitUnicodeString(&device_link, FORT_DOS_DEVICE_NAME);
        IoDeleteSymbolicLink(&device_link);

        IoDeleteDevice(driver->DeviceObject);
        driver->DeviceObject = NULL;
    }
}

static NTSTATUS fort_driver_load(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    NTSTATUS status;

    UNUSED(reg_path);

    // Use NX Non-Paged Pool
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    /* Wait for BFE to start */
    status = fort_bfe_wait();
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING device_name;
    RtlInitUnicodeString(&device_name, FORT_NT_DEVICE_NAME);

    PDEVICE_OBJECT device_obj;
    status = IoCreateDevice(
            driver, sizeof(FORT_DEVICE), &device_name, FORT_DEVICE_TYPE, 0, FALSE, &device_obj);
    if (!NT_SUCCESS(status))
        return status;

    driver->MajorFunction[IRP_MJ_CREATE] = fort_device_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = fort_device_close;
    driver->MajorFunction[IRP_MJ_CLEANUP] = fort_device_cleanup;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = fort_device_control;
    driver->DriverUnload = fort_driver_unload;

    UNICODE_STRING device_link;

    RtlInitUnicodeString(&device_link, FORT_DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&device_link, &device_name);
    if (!NT_SUCCESS(status))
        return status;

    device_obj->Flags |= DO_BUFFERED_IO;

    return fort_device_load(device_obj);
}

NTSTATUS __declspec(dllexport) DriverCallbackEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    NTSTATUS status;

    status = fort_driver_load(driver, reg_path);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Entry: Error: %x\n", status);
        fort_driver_unload(driver);
    }

    return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    return DriverCallbackEntry(driver, reg_path);
}
