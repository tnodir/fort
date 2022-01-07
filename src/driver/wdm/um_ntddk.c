#include "um_ntddk.h"

NTSTATUS IoQueryFullDriverPath(PDRIVER_OBJECT driverObject, PUNICODE_STRING fullPath)
{
    UNUSED(driverObject);
    UNUSED(fullPath);
    return STATUS_SUCCESS;
}

NTSTATUS PsSetCreateProcessNotifyRoutineEx(
        PCREATE_PROCESS_NOTIFY_ROUTINE_EX notifyRoutine, BOOLEAN remove)
{
    UNUSED(notifyRoutine);
    UNUSED(remove);
    return STATUS_SUCCESS;
}
