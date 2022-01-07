/* Fort Firewall Process Tree */

#include "fortps.h"

#include "fortcb.h"

static void NTAPI fort_pstree_notify(
        PEPROCESS process, HANDLE processId, PPS_CREATE_NOTIFY_INFO createInfo)
{
    UNUSED(process);

    if (createInfo == NULL) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: PsTree: pid=%d CLOSED\n",
                (DWORD) (ptrdiff_t) processId);
        return;
    }

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "FORT: PsTree: pid=%d ppid=%d pupid=%d FileOpenNameAvailable=%d "
            "IsSubsystemProcess=%d\n",
            (DWORD) (ptrdiff_t) processId, (DWORD) (ptrdiff_t) createInfo->ParentProcessId,
            (DWORD) (ptrdiff_t) createInfo->CreatingThreadId.UniqueProcess,
            createInfo->FileOpenNameAvailable, createInfo->IsSubsystemProcess);

    if (createInfo->ImageFileName != NULL) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: PsTree: pid=%d IMG=[%wZ]\n",
                (DWORD) (ptrdiff_t) processId, createInfo->ImageFileName);
    }

    if (createInfo->CommandLine != NULL) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: PsTree: pid=%d CMD=[%wZ]\n",
                (DWORD) (ptrdiff_t) processId, createInfo->CommandLine);
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
