/* Fort Firewall Driver Loader */

#include "fortdl.h"

#include "../fortutl.h"

#include "fortimg.h"
#include "fortmm.h"

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

    UnloadModule(&g_loader.module);
}

static NTSTATUS fort_loader_entry(PDRIVER_OBJECT driver, PUNICODE_STRING regPath)
{
    NTSTATUS status;

    status = CallModuleEntry(&g_loader.module, driver, regPath);
    if (!NT_SUCCESS(status))
        return status;

    /* Chain the driver unloaders */
    g_loader.driver_unload = driver->DriverUnload;
    driver->DriverUnload = fort_loader_unload;

    return status;
}

static NTSTATUS fort_loader_init(PUNICODE_STRING driverPath)
{
    NTSTATUS status;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: [%wZ]\n", driverPath);

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
                    "FORT: Loader File Read: Error: %x size=%d [%wZ]\n", status, dataSize,
                    driverPath);
            return status;
        }
    }

    /* Prepare the driver image */
    PUCHAR payload = NULL;
    DWORD payloadSize = 0;
    status = fort_image_payload(data, dataSize, &payload, &payloadSize);

    /* Free the driver file's allocated data */
    fort_mem_free(data, FORT_LOADER_POOL_TAG);

    /* Load the module driver */
    if (NT_SUCCESS(status)) {
        status = LoadModuleFromMemory(&g_loader.module, payload, payloadSize);
    }

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                "FORT: Loader Image Payload: Error: %x\n", status);
    }

    return status;
}

NTSTATUS
#if defined(FORT_DRIVER)
DriverEntry
#else
DriverLoaderEntry
#endif
        (PDRIVER_OBJECT driver, PUNICODE_STRING regPath)
{
    NTSTATUS status;

    UNICODE_STRING driverPath;
    status = fort_driver_path(driver, regPath, &driverPath);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Entry: Path Error: %x\n",
                status);
        return status;
    }

    /* Initialize module driver */
    status = fort_loader_init(&driverPath);

    /* Free the allocated driver path */
    ExFreePool(driverPath.Buffer);

    /* Call the driver entry */
    if (NT_SUCCESS(status)) {
        status = fort_loader_entry(driver, regPath);
    }

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Entry: Error: %x\n",
                status);

        fort_loader_unload(driver);
    }

    return status;
}
