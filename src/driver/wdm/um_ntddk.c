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

NTSTATUS KeExpandKernelStackAndCallout(PEXPAND_STACK_CALLOUT callout, PVOID parameter, SIZE_T size)
{
    UNUSED(callout);
    UNUSED(parameter);
    UNUSED(size);
    return STATUS_SUCCESS;
}

LONG KeSetBasePriorityThread(PVOID threadObject, LONG increment)
{
    UNUSED(threadObject);
    UNUSED(increment);
    return 0;
}
