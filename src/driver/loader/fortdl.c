/* Fort Firewall Driver Loader */

#include "fortdl.h"

#include "../fortutl.h"

#include "fortimg.h"

static void fortdl_init(PDRIVER_OBJECT driver, PVOID context, ULONG count)
{
    NTSTATUS status;

    UNUSED(count);

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: %d\n", count);

    /* Load the driver file */
    PUCHAR data = NULL;
    DWORD dataSize = 0;
    {
        HANDLE fileHandle;
        status = fort_file_open(context, &fileHandle);

        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                "FORT: Loader File Open: %w status=%d\n", (PCWSTR) context, status);

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

    /* Prepare the driver image */
    PUCHAR image = NULL;
    DWORD imageSize = 0;
    if (NT_SUCCESS(status)) {
        status = fort_image_load(data, dataSize, &image, &imageSize);

        /* Free the driver file's allocated data */
        fort_mem_free(data, FORT_LOADER_POOL_TAG);
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
