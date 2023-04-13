/* Fort Firewall Driver Loader */

#include "fortdl.h"

#include "../fortutl.h"
#include "../proxycb/fortpcb_drv.h"
#include "../proxycb/fortpcb_src.h"

#include "fortimg.h"
#include "fortmm.h"

typedef struct fort_loader
{
    LOADEDMODULE module;

    PDRIVER_UNLOAD DriverUnload;
} FORT_LOADER, *PFORT_LOADER;

static FORT_LOADER g_loader;

static void fort_loader_unload(PDRIVER_OBJECT driver)
{
    if (g_loader.DriverUnload != NULL) {
        g_loader.DriverUnload(driver);
        g_loader.DriverUnload = NULL;
    }

    UnloadModule(&g_loader.module);
}

static NTSTATUS fort_loader_entry(PDRIVER_OBJECT driver, PUNICODE_STRING regPath)
{
    NTSTATUS status;

    /* Prepare the major functions */
    fort_proxycb_drv_prepare(driver->MajorFunction);

    /* Prepare the proxy callbacks */
    FORT_PROXYCB_INFO cbInfo;
    fort_proxycb_src_prepare(&cbInfo);

    /* Setup the module's callbacks */
    status = SetupModuleCallbacks(&g_loader.module, &cbInfo);
    if (!NT_SUCCESS(status))
        return status;

    /* Setup the proxy callbacks */
    fort_proxycb_src_setup(&cbInfo);

    /* Run the module's entry function */
    status = CallModuleEntry(&g_loader.module, driver, regPath);
    if (!NT_SUCCESS(status))
        return status;

    /* Proxy the driver's unload function */
    g_loader.DriverUnload = driver->DriverUnload;
    driver->DriverUnload = fort_loader_unload;

    /* Proxy the driver's major functions */
    fort_proxycb_drv_setup(driver->MajorFunction);

    return status;
}

static NTSTATUS fort_loader_init(PUNICODE_STRING driverPath)
{
    NTSTATUS status;

#ifdef FORT_DEBUG
    LOG("Loader Init: [%wZ]\n", driverPath);
#endif

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
            LOG("Loader File Read: Error: %x size=%d [%wZ]\n", status, dataSize, driverPath);
            return status;
        }
    }

    /* Prepare the driver image */
    PUCHAR payload = NULL;
    DWORD payloadSize = 0;
    status = fort_image_payload(data, dataSize, &payload, &payloadSize);

    /* Load the module driver */
    if (NT_SUCCESS(status)) {
        status = LoadModuleFromMemory(&g_loader.module, payload, payloadSize);
    }

    /* Free the driver file's allocated data */
    fort_mem_free(data, FORT_LOADER_POOL_TAG);

    if (!NT_SUCCESS(status)) {
        LOG("Loader Image Payload: Error: %x\n", status);
    }

    return status;
}

#if defined(FORT_DRIVER)
DRIVER_INITIALIZE DriverEntry;
#endif

NTSTATUS
#if defined(FORT_DRIVER)
DriverEntry
#else
DriverLoaderEntry
#endif
        (PDRIVER_OBJECT driver, PUNICODE_STRING regPath)
{
    NTSTATUS status;

    // Use NX Non-Paged Pool
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    UNICODE_STRING driverPath;
    status = fort_driver_path(driver, regPath, &driverPath);

    if (!NT_SUCCESS(status)) {
        LOG("Loader Entry: Path Error: %x\n", status);
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
        LOG("Loader Entry: Error: %x\n", status);

        fort_loader_unload(driver);
    }

    return status;
}
