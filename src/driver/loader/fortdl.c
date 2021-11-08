/* Fort Firewall Driver Loader */

#include "fortdl.h"

#include "../fortutl.h"

#include "fortimg.h"

typedef struct fort_loader
{
    LOADEDMODULE module;

    PDRIVER_UNLOAD driver_unload;
} FORT_LOADER, *PFORT_LOADER;

static FORT_LOADER g_loader;

static void fort_loader_unload(PDRIVER_OBJECT driver)
{
    if (g_loader.driver_unload) {
        g_loader.driver_unload(driver);
    }

    fort_image_unload(&g_loader.module);
}

static NTSTATUS fort_loader_init(PDRIVER_OBJECT driver, PWSTR driverPath)
{
    NTSTATUS status;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: %ws irql=%d\n",
            driverPath, KeGetCurrentIrql());

    /* Load the driver file */
    PUCHAR data = NULL;
    DWORD dataSize = 0;
    {
        HANDLE fileHandle;
        status = fort_file_open(driverPath, &fileHandle);

        if (NT_SUCCESS(status)) {
            status = fort_file_read(fileHandle, FORT_LOADER_POOL_TAG, &data, &dataSize);

            ZwClose(fileHandle);
        }

        if (!NT_SUCCESS(status)) {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: Loader File Read: Error: %x size=%d [%ws]\n", status, dataSize,
                    driverPath);
            return status;
        }
    }

    /* Prepare the driver image */
    status = fort_image_load(data, dataSize, &g_loader.module);

    /* Free the driver file's allocated data */
    fort_mem_free(data, FORT_LOADER_POOL_TAG);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Image Load: Error: %x\n",
                status);
        return status;
    }

    /* Run the driver entry */

    status = STATUS_UNSUCCESSFUL;

    return status;
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
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Entry: Path Error: %x\n",
                status);
        return status;
    }

    status = fort_loader_init(driver, driverPath);

    /* Free the allocated driver path */
    ExFreePool(driverPath);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Entry: Error: %x\n",
                status);
    }

    return status;
}
