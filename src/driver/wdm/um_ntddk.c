#include "um_ntddk.h"

NTSTATUS IoQueryFullDriverPath(PDRIVER_OBJECT DriverObject, PUNICODE_STRING FullPath)
{
    UNUSED(DriverObject);
    UNUSED(FullPath);
    return 0;
}

VOID IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
        PDRIVER_REINITIALIZE DriverReinitializationRoutine, PVOID Context)
{
    UNUSED(DriverObject);
    UNUSED(DriverReinitializationRoutine);
    UNUSED(Context);
}
