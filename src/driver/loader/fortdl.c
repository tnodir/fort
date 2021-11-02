/* Fort Firewall Driver Loader */

#include "fortdl.h"

#include "../fortutl.h"

static NTSTATUS fortdl_load_image(PUCHAR data, DWORD dataSize, PUCHAR *image)
{
    NTSTATUS status;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Load Image: %d\n", dataSize);

    return STATUS_SUCCESS;
}

static void fortdl_init(PDRIVER_OBJECT driver, PVOID context, ULONG count)
{
    NTSTATUS status;

    UNUSED(context);
    UNUSED(count);

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: %d\n", count);

    /* Load the driver file */
    PUCHAR data = NULL;
    DWORD dataSize = 0;
    {
        HANDLE fileHandle;
        status = fort_file_open(context, &fileHandle);

        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                "FORT: Loader File Load: %w status=%d\n", (PCWSTR) context, status);

        if (NT_SUCCESS(status)) {
            status = fort_file_read(fileHandle, FORT_LOADER_POOL_TAG, &data, &dataSize);
            ZwClose(fileHandle);
        }

        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                "FORT: Loader File Read: %w status=%d size=%d\n", (PCWSTR) context, status,
                dataSize);

        /* Free the allocated driver path */
        ExFreePool(context);
    }

    // Prepare the driver image
    PUCHAR image = NULL;
    if (NT_SUCCESS(status)) {
        status = fortdl_load_image(data, dataSize, &image);

        /* Free the allocated driver file data */
        fortdl_free(data);
    }

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(
                DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: Error: %x\n", status);
    }
}

NTSTATUS
#if defined(FORT_DRIVER)
DriverEntry
#else
DriverLoaderEntry
#endif
        (PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    NTSTATUS status;

    PWSTR driverPath = NULL;
    status = fort_driver_path(driver, reg_path, &driverPath);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Entry: Error: %x\n",
                status);
        return status;
    }

    /* Delay the initialization until other drivers have finished loading */
    IoRegisterDriverReinitialization(driver, fortdl_init, driverPath);

    return STATUS_SUCCESS;
}