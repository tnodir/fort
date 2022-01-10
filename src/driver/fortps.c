/* Fort Firewall Process Tree */

#include "fortps.h"

#include "fortcb.h"
#include "fortutl.h"

static BOOL fort_pstree_svchost_check(
        PCUNICODE_STRING path, PCUNICODE_STRING commandLine, PUNICODE_STRING serviceName)
{
    const USHORT svchostSize = sizeof(L"svchost.exe") - sizeof(WCHAR); /* skip terminating zero */
    const USHORT svchostCount = svchostSize / sizeof(WCHAR);
    const USHORT sys32Size = path->Length - svchostSize;
    const USHORT sys32Count = sys32Size / sizeof(WCHAR);

    if (_wcsnicmp(path->Buffer + sys32Count, L"svchost.exe", svchostCount) != 0)
        return FALSE;

    PCUNICODE_STRING sys32Path = fort_system32_path();
    if (sys32Size != sys32Path->Length
            || _wcsnicmp(path->Buffer, sys32Path->Buffer, sys32Count) != 0)
        return FALSE;

    PWCHAR argp = wcsstr(commandLine->Buffer, L"-s ");
    if (argp == NULL)
        return FALSE;

    argp += (sizeof(L"-s ") - sizeof(WCHAR)) / sizeof(WCHAR); /* skip terminating zero */

    PCWCHAR endp = wcschr(argp, L' ');
    if (endp == NULL) {
        endp = (PCWCHAR) ((PCHAR) commandLine->Buffer + commandLine->Length);
    }

    serviceName->Length = (USHORT) ((PCHAR) endp - (PCHAR) argp);
    serviceName->MaximumLength = serviceName->Length;
    serviceName->Buffer = argp;

    return TRUE;
}

static void NTAPI fort_pstree_notify(
        PEPROCESS process, HANDLE processId, PPS_CREATE_NOTIFY_INFO createInfo)
{
    UNUSED(process);

    const DWORD pid = (DWORD) (ptrdiff_t) processId;
    const DWORD ppid = (DWORD) (ptrdiff_t) createInfo->ParentProcessId;

    if (createInfo == NULL) {
#ifdef FORT_DEBUG
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: PsTree: pid=%d CLOSED\n", pid);
#endif
        return;
    }

    if (createInfo->ImageFileName == NULL || createInfo->CommandLine == NULL)
        return;

#ifdef FORT_DEBUG
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "FORT: PsTree: pid=%d ppid=%d IMG=[%wZ] CMD=[%wZ]\n", pid, ppid,
            createInfo->ImageFileName, createInfo->CommandLine);
#endif

    /* TODO: createInfo->CreationStatus = STATUS_ACCESS_DENIED; */

    UNICODE_STRING serviceName;
    if (fort_pstree_svchost_check(
                createInfo->ImageFileName, createInfo->CommandLine, &serviceName)) {
        // TODO
    }
}

FORT_API void fort_pstree_open(PFORT_PSTREE ps_tree)
{
    NTSTATUS status;

    status = PsSetCreateProcessNotifyRoutineEx(
            FORT_CALLBACK(
                    FORT_PSTREE_NOTIFY, PCREATE_PROCESS_NOTIFY_ROUTINE_EX, fort_pstree_notify),
            FALSE);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                "FORT: PsTree: PsSetCreateProcessNotifyRoutineEx Error: %x\n", status);
    }
}

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree)
{
    PsSetCreateProcessNotifyRoutineEx(
            FORT_CALLBACK(
                    FORT_PSTREE_NOTIFY, PCREATE_PROCESS_NOTIFY_ROUTINE_EX, fort_pstree_notify),
            TRUE);
}
