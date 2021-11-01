#include "um_ntddk.h"

NTSTATUS IoQueryFullDriverPath(PDRIVER_OBJECT DriverObject, PUNICODE_STRING FullPath)
{
    UNUSED(DriverObject);
    UNUSED(FullPath);
    return STATUS_SUCCESS;
}

VOID IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
        PDRIVER_REINITIALIZE DriverReinitializationRoutine, PVOID Context)
{
    UNUSED(DriverObject);
    UNUSED(DriverReinitializationRoutine);
    UNUSED(Context);
}
