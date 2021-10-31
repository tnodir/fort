/* Fort Firewall Driver Loader */

#include "fortdl.h"

static void fortdl_init(PDRIVER_OBJECT driver, PVOID context, ULONG count)
{
    UNUSED(context);
    UNUSED(count);

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Entry: %d\n", count);
}

NTSTATUS
#if defined(FORT_DRIVER)
DriverEntry
#else
DriverLoaderEntry
#endif
        (PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    UNUSED(reg_path);

    /* Delay the initialization until other drivers have finished loading */
    IoRegisterDriverReinitialization(driver, fortdl_init, NULL);

    return STATUS_SUCCESS;
}
