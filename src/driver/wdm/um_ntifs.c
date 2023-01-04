#include "um_ntifs.h"

NTSTATUS RtlDowncaseUnicodeString(PUNICODE_STRING destinationString, PCUNICODE_STRING sourceString,
        BOOLEAN allocateDestinationString)
{
    UNUSED(destinationString);
    UNUSED(sourceString);
    UNUSED(allocateDestinationString);
    return STATUS_SUCCESS;
}

NTSTATUS PsLookupProcessByProcessId(HANDLE processId, PEPROCESS *process)
{
    UNUSED(processId);
    UNUSED(process);
    return STATUS_SUCCESS;
}

NTSTATUS ObOpenObjectByPointer(PVOID object, ULONG handleAttributes,
        PACCESS_STATE passedAccessState, ACCESS_MASK desiredAccess, POBJECT_TYPE objectType,
        KPROCESSOR_MODE accessMode, PHANDLE handle)
{
    UNUSED(object);
    UNUSED(handleAttributes);
    UNUSED(passedAccessState);
    UNUSED(desiredAccess);
    UNUSED(objectType);
    UNUSED(accessMode);
    UNUSED(handle);
    return STATUS_SUCCESS;
}

void KeStackAttachProcess(PRKPROCESS process, PRKAPC_STATE apcState)
{
    UNUSED(process);
    UNUSED(apcState);
}

void KeUnstackDetachProcess(PRKAPC_STATE apcState)
{
    UNUSED(apcState);
}

ULONG RtlRandomEx(PULONG seed)
{
    UNUSED(seed);
    return 0;
}
