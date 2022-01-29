/* Fort Firewall Process Tree */

#include "fortps.h"

#include "fortcb.h"
#include "fortutl.h"

#define FORT_PSTREE_POOL_TAG 'PwfF'

#define FORT_PSTREE_NAME_LEN_MAX (64 * sizeof(WCHAR))

typedef struct fort_psname
{
    USHORT size;
    WCHAR data[1];
} FORT_PSNAME, *PFORT_PSNAME;

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_psnode
{
    struct fort_psnode *next;
    struct fort_psnode *prev;

    PFORT_PSNAME name;

    UINT32 process_id;

    UINT16 flags;
    UINT16 parent_index;

    UINT16 next_sibling_index;
    UINT16 prev_sibling_index;

    UINT16 next_child_index;
    UINT16 prev_child_index;
} FORT_PSNODE, *PFORT_PSNODE;

typedef struct _SYSTEM_PROCESSES
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    ULONG Reserved1[6];
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    SIZE_T ProcessId;
    SIZE_T ParentProcessId;
    ULONG HandleCount;
    ULONG SessionId;
} SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;

#if !defined(SystemProcessInformation)
#    define SystemProcessInformation 5
#endif

#if defined(FORT_DRIVER)
NTSTATUS NTAPI ZwQuerySystemInformation(ULONG systemInformationClass, PVOID systemInformation,
        ULONG systemInformationLength, PULONG returnLength);
#endif

static NTSTATUS fort_pstree_enum_processes_loop(
        PFORT_PSTREE ps_tree, PSYSTEM_PROCESSES processEntry)
{
    NTSTATUS status = STATUS_SUCCESS;

    for (;;) {
        const DWORD pid = (DWORD) processEntry->ProcessId;
        const DWORD ppid = (DWORD) processEntry->ParentProcessId;

        // TODO

        if (processEntry->NextEntryOffset == 0)
            break;

        processEntry = (PSYSTEM_PROCESSES) ((PUCHAR) processEntry + processEntry->NextEntryOffset);
    }

    return status;
}

static NTSTATUS fort_pstree_enum_processes(PFORT_PSTREE ps_tree)
{
    NTSTATUS status;

    ULONG bufferSize;
    status = ZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &bufferSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
        return status;

    bufferSize *= 2; /* for possible new created processes/threads */

    PVOID buffer = fort_mem_alloc(bufferSize, FORT_PSTREE_POOL_TAG);
    if (buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    status = ZwQuerySystemInformation(SystemProcessInformation, buffer, bufferSize, &bufferSize);
    if (NT_SUCCESS(status)) {
        status = fort_pstree_enum_processes_loop(ps_tree, buffer);
    }

    fort_mem_free(buffer, FORT_PSTREE_POOL_TAG);

    return status;
}

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

    const USHORT nameLen = (USHORT) ((PCHAR) endp - (PCHAR) argp);
    if (nameLen >= FORT_PSTREE_NAME_LEN_MAX)
        return FALSE;

    serviceName->Length = nameLen;
    serviceName->MaximumLength = nameLen;
    serviceName->Buffer = argp;

    return TRUE;
}

static void NTAPI fort_pstree_notify(
        PEPROCESS process, HANDLE processId, PPS_CREATE_NOTIFY_INFO createInfo)
{
    UNUSED(process);

    const DWORD pid = (DWORD) (ptrdiff_t) processId;

    if (createInfo == NULL) {
#ifdef FORT_DEBUG
        LOG("PsTree: pid=%d CLOSED\n", pid);
#endif
        return;
    }

    if (createInfo->ImageFileName == NULL || createInfo->CommandLine == NULL)
        return;

    const DWORD ppid = (DWORD) (ptrdiff_t) createInfo->ParentProcessId;

#ifdef FORT_DEBUG
    LOG("PsTree: pid=%d ppid=%d IMG=[%wZ] CMD=[%wZ]\n", pid, ppid,
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

    fort_pool_list_init(&ps_tree->pool_list);
    tommy_list_init(&ps_tree->free_nodes);

    tommy_arrayof_init(&ps_tree->procs, sizeof(FORT_PSNODE));
    tommy_hashdyn_init(&ps_tree->procs_map);

    KeInitializeSpinLock(&ps_tree->lock);

    status = PsSetCreateProcessNotifyRoutineEx(
            FORT_CALLBACK(
                    FORT_PSTREE_NOTIFY, PCREATE_PROCESS_NOTIFY_ROUTINE_EX, fort_pstree_notify),
            FALSE);
    if (!NT_SUCCESS(status)) {
        LOG("PsTree: PsSetCreateProcessNotifyRoutineEx Error: %x\n", status);
        return;
    }

    status = fort_pstree_enum_processes(ps_tree);
    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Enum Processes Error: %x\n", status);
        return;
    }
}

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree)
{
    PsSetCreateProcessNotifyRoutineEx(
            FORT_CALLBACK(
                    FORT_PSTREE_NOTIFY, PCREATE_PROCESS_NOTIFY_ROUTINE_EX, fort_pstree_notify),
            TRUE);

    KLOCK_QUEUE_HANDLE lock_queue;

    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);

    fort_pool_done(&ps_tree->pool_list);

    tommy_arrayof_done(&ps_tree->procs);
    tommy_hashdyn_done(&ps_tree->procs_map);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}
