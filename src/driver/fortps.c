/* Fort Firewall Process Tree */

#include "fortps.h"

#include "fortcb.h"
#include "fortdev.h"
#include "fortutl.h"

#define FORT_PSTREE_POOL_TAG 'PwfF'

#define FORT_SVCHOST_PREFIX L"\\svchost\\"
#define FORT_SVCHOST_EXE    L"svchost.exe"

#define FORT_PSTREE_NAME_LEN_MAX    (120 * sizeof(WCHAR))
#define FORT_PSTREE_NAMES_POOL_SIZE (4 * 1024)

#define FORT_PSNAME_DATA_OFF offsetof(FORT_PSNAME, data)

struct fort_psname
{
    UINT16 size;
    WCHAR data[1];
};

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_psnode
{
    struct fort_psnode *next;
    struct fort_psnode *prev;

    PFORT_PSNAME ps_name; /* tommy_hashdyn_node::data */

    tommy_key_t pid_hash; /* tommy_hashdyn_node::index */

    UINT32 process_id;
    UINT32 parent_process_id;

    UINT16 flags;
    UINT16 conf_chn;
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

typedef struct _PEB_LDR_DATA
{
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef VOID(NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)(VOID);

typedef struct _PEB
{
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID Reserved4[3];
    PVOID AtlThunkSListPtr;
    PVOID Reserved5;
    ULONG Reserved6;
    PVOID Reserved7;
    ULONG Reserved8;
    ULONG AtlThunkSListPtr32;
    PVOID Reserved9[45];
    BYTE Reserved10[96];
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE Reserved11[128];
    PVOID Reserved12[1];
    ULONG SessionId;
} PEB, *PPEB;

NTSTATUS NTAPI ZwQuerySystemInformation(ULONG systemInformationClass, PVOID systemInformation,
        ULONG systemInformationLength, PULONG returnLength);

NTSTATUS NTAPI ZwQueryInformationProcess(HANDLE processHandle, ULONG processInformationClass,
        PVOID processInformation, ULONG processInformationLength, PULONG returnLength);

#endif

#define fort_pstree_get_proc(ps_tree, index)                                                       \
    ((PFORT_PSNODE) tommy_arrayof_ref(&(ps_tree)->procs, (index)))

static PFORT_PSNAME fort_pstree_name_new(PFORT_PSTREE ps_tree, UINT16 name_size)
{
    PFORT_PSNAME ps_name = fort_pool_malloc(&ps_tree->pool_list,
            FORT_PSNAME_DATA_OFF + name_size + sizeof(WCHAR)); /* include terminating zero */
    if (ps_name != NULL) {
        ps_name->size = name_size;
        ps_name->data[name_size / sizeof(WCHAR)] = L'\0';
    }
    return ps_name;
}

static void fort_pstree_name_del(PFORT_PSTREE ps_tree, PFORT_PSNAME ps_name)
{
    if (ps_name != NULL) {
        fort_pool_free(&ps_tree->pool_list, ps_name);
    }
}

static BOOL fort_pstree_svchost_check(
        PCUNICODE_STRING path, PCUNICODE_STRING commandLine, PUNICODE_STRING serviceName)
{
    const USHORT svchostSize = sizeof(FORT_SVCHOST_EXE) - sizeof(WCHAR); /* skip terminating zero */
    const USHORT svchostCount = svchostSize / sizeof(WCHAR);
    const USHORT sys32Size = path->Length - svchostSize;
    const USHORT sys32Count = sys32Size / sizeof(WCHAR);

    if (_wcsnicmp(path->Buffer + sys32Count, FORT_SVCHOST_EXE, svchostCount) != 0)
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

static PFORT_PSNAME fort_pstree_add_service_name(
        PFORT_PSTREE ps_tree, PCUNICODE_STRING path, PCUNICODE_STRING commandLine)
{
    UNICODE_STRING serviceName;
    if (!fort_pstree_svchost_check(path, commandLine, &serviceName))
        return NULL;

    UNICODE_STRING svchostPrefix;
    RtlInitUnicodeString(&svchostPrefix, FORT_SVCHOST_PREFIX);

    PFORT_PSNAME ps_name = fort_pstree_name_new(ps_tree, svchostPrefix.Length + serviceName.Length);
    if (ps_name != NULL) {
        PCHAR data = (PCHAR) &ps_name->data;
        RtlCopyMemory(data, svchostPrefix.Buffer, svchostPrefix.Length);

        UNICODE_STRING nameString;
        nameString.Length = serviceName.Length;
        nameString.MaximumLength = serviceName.Length;
        nameString.Buffer = (PWSTR) (data + svchostPrefix.Length);
        RtlDowncaseUnicodeString(&nameString, &serviceName, FALSE);
    }

    return ps_name;
}

static PFORT_PSNODE fort_pstree_proc_new(
        PFORT_PSTREE ps_tree, PFORT_PSNAME ps_name, tommy_key_t pid_hash)
{
    tommy_arrayof *procs = &ps_tree->procs;
    tommy_hashdyn *procs_map = &ps_tree->procs_map;

    tommy_hashdyn_node *proc_node = tommy_list_tail(&ps_tree->free_procs);

    if (proc_node != NULL) {
        tommy_list_remove_existing(&ps_tree->free_procs, proc_node);
    } else {
        const UINT16 index = ps_tree->procs_n;

        tommy_arrayof_grow(procs, index + 1);

        proc_node = tommy_arrayof_ref(procs, index);
    }

    tommy_hashdyn_insert(procs_map, proc_node, ps_name, pid_hash);

    ++ps_tree->procs_n;

    return (PFORT_PSNODE) proc_node;
}

static void fort_pstree_proc_del(PFORT_PSTREE ps_tree, PFORT_PSNODE proc)
{
    --ps_tree->procs_n;

    // Delete from pool
    fort_pstree_name_del(ps_tree, proc->ps_name);

    // Delete from procs map
    tommy_hashdyn_remove_existing(&ps_tree->procs_map, (tommy_hashdyn_node *) proc);

    tommy_list_insert_tail_check(&ps_tree->free_procs, (tommy_node *) proc);
}

static PFORT_PSNODE fort_pstree_find_proc_hash(
        PFORT_PSTREE ps_tree, DWORD processId, tommy_key_t pid_hash)
{
    PFORT_PSNODE node = (PFORT_PSNODE) tommy_hashdyn_bucket(&ps_tree->procs_map, pid_hash);

    while (node != NULL) {
        if (node->process_id == processId) {
            return node;
        }

        node = node->next;
    }

    return NULL;
}

static PFORT_PSNODE fort_pstree_find_proc(PFORT_PSTREE ps_tree, DWORD processId)
{
    const tommy_key_t pid_hash = (tommy_key_t) tommy_hash_u32(0, &processId, sizeof(DWORD));

    return fort_pstree_find_proc_hash(ps_tree, processId, pid_hash);
}

static void fort_pstree_handle_new_proc(PFORT_PSTREE ps_tree, PCUNICODE_STRING path,
        PCUNICODE_STRING commandLine, tommy_key_t pid_hash, DWORD processId, DWORD parentProcessId)
{
    PFORT_PSNAME ps_name = fort_pstree_add_service_name(ps_tree, path, commandLine);

    PFORT_PSNODE proc = fort_pstree_proc_new(ps_tree, ps_name, pid_hash);
    if (proc == NULL) {
        fort_pstree_name_del(ps_tree, ps_name);
        return;
    }

    proc->process_id = processId;
    proc->parent_process_id = parentProcessId;

    proc->flags = 0;
    proc->conf_chn = 0;
}

static void NTAPI fort_pstree_notify(
        PEPROCESS process, HANDLE processHandle, PPS_CREATE_NOTIFY_INFO createInfo)
{
    PFORT_PSTREE ps_tree = &fort_device()->ps_tree;

    UNUSED(process);

    const DWORD processId = (DWORD) (ptrdiff_t) processHandle;
    const tommy_key_t pid_hash = (tommy_key_t) tommy_hash_u32(0, &processId, sizeof(DWORD));

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);

    PFORT_PSNODE proc = fort_pstree_find_proc_hash(ps_tree, processId, pid_hash);

    if (createInfo == NULL) {
        /* Process Closed */
#ifdef FORT_DEBUG
        LOG("PsTree: pid=%d CLOSED\n", processId);
#endif

        if (proc != NULL) {
            fort_pstree_proc_del(ps_tree, proc);
        }
    }
    /* Process Created */
    else if (createInfo->ImageFileName != NULL && createInfo->CommandLine != NULL) {
        const DWORD parentProcessId = (DWORD) (ptrdiff_t) createInfo->ParentProcessId;

#ifdef FORT_DEBUG
        LOG("PsTree: pid=%d ppid=%d IMG=[%wZ] CMD=[%wZ]\n", processId, parentProcessId,
                createInfo->ImageFileName, createInfo->CommandLine);
#endif

        if (proc == NULL) {
            UNICODE_STRING path = *createInfo->ImageFileName;
            fort_path_prefix_adjust(&path);

            fort_pstree_handle_new_proc(
                    ps_tree, &path, createInfo->CommandLine, pid_hash, processId, parentProcessId);
        }
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static void fort_pstree_update(PFORT_PSTREE ps_tree, BOOL active)
{
    if (ps_tree->active == active)
        return;

    ps_tree->active = (UINT8) active;

    const NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(
            FORT_CALLBACK(
                    FORT_PSTREE_NOTIFY, PCREATE_PROCESS_NOTIFY_ROUTINE_EX, fort_pstree_notify),
            /*remove=*/!active);

    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Update Error: %x\n", status);
    }
}

FORT_API void fort_pstree_open(PFORT_PSTREE ps_tree)
{
    fort_pool_list_init(&ps_tree->pool_list);
    fort_pool_init(&ps_tree->pool_list, FORT_PSTREE_NAMES_POOL_SIZE);

    tommy_list_init(&ps_tree->free_procs);

    tommy_arrayof_init(&ps_tree->procs, sizeof(FORT_PSNODE));
    tommy_hashdyn_init(&ps_tree->procs_map);

    KeInitializeSpinLock(&ps_tree->lock);

    fort_pstree_update(ps_tree, TRUE); /* Start process monitor */
}

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree)
{
    fort_pstree_update(ps_tree, FALSE); /* Stop process monitor */

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        fort_pool_done(&ps_tree->pool_list);

        tommy_arrayof_done(&ps_tree->procs);
        tommy_hashdyn_done(&ps_tree->procs_map);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API PFORT_PSNAME fort_pstree_get_proc_name(
        PFORT_PSTREE ps_tree, DWORD processId, PUNICODE_STRING path)
{
    PFORT_PSNAME ps_name = NULL;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        PFORT_PSNODE proc = fort_pstree_find_proc(ps_tree, processId);
        if (proc != NULL && proc->ps_name != NULL) {
            ps_name = proc->ps_name;

            path->Length = ps_name->size;
            path->MaximumLength = ps_name->size;
            path->Buffer = ps_name->data;
        }
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return ps_name;
}

static HANDLE OpenProcessById(DWORD processId)
{
    if (processId == 0 || processId == 4)
        return NULL; // skip "System Idle Process" (0) and "System" (4) processes

    NTSTATUS status;

    PEPROCESS peProcess = NULL;
    status = PsLookupProcessByProcessId((HANDLE) (ptrdiff_t) processId, &peProcess);
    if (!NT_SUCCESS(status) || peProcess == NULL) {
        LOG("PsTree: Lookup Process Error: %x\n", status);
        return NULL;
    }

    HANDLE processHandle = NULL;
    status = ObOpenObjectByPointer(peProcess, 0, NULL, 0, 0, KernelMode, &processHandle);

    ObDereferenceObject(peProcess);

    if (!NT_SUCCESS(status) || processHandle == NULL) {
        LOG("PsTree: Open Process Object Error: %x\n", status);
        return NULL;
    }

    return processHandle;
}

static PWCHAR GetUnicodeStringBuffer(PCUNICODE_STRING string, PRTL_USER_PROCESS_PARAMETERS params)
{
#ifdef _X86_
    if ((PCHAR) string->Buffer < (PCHAR) params) {
        return (PWCHAR) ((PCHAR) params + (DWORD) (ptrdiff_t) string->Buffer);
    }
#else
    UNUSED(params);
#endif

    return string->Buffer;
}

static NTSTATUS GetCurrentProcessPathArgs(PUNICODE_STRING path, PUNICODE_STRING commandLine)
{
    NTSTATUS status;

    PROCESS_BASIC_INFORMATION procBasicInfo;
    status = ZwQueryInformationProcess(ZwCurrentProcess(), ProcessBasicInformation, &procBasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION), NULL);
    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Query Process Error: %x\n", status);
        return status;
    }

    if (procBasicInfo.PebBaseAddress == NULL) {
        LOG("PsTree: Query Process Error: PebBaseAddress\n");
        return STATUS_INVALID_ADDRESS;
    }

    PRTL_USER_PROCESS_PARAMETERS params = procBasicInfo.PebBaseAddress->ProcessParameters;

    path->Length = params->ImagePathName.Length;
    path->MaximumLength = params->ImagePathName.Length;
    path->Buffer = GetUnicodeStringBuffer(&params->ImagePathName, params);

    commandLine->Length = params->CommandLine.Length;
    commandLine->MaximumLength = params->CommandLine.Length;
    commandLine->Buffer = GetUnicodeStringBuffer(&params->CommandLine, params);

    return STATUS_SUCCESS;
}

static void fort_pstree_attach_process(PSYSTEM_PROCESSES processEntry, HANDLE processHandle)
{
    NTSTATUS status;

    PRKPROCESS process;
    status = ObReferenceObjectByHandle(
            processHandle, 0, *PsProcessType, KernelMode, (PVOID *) &process, NULL);
    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Attach Process Error: %x\n", status);
        return;
    }

    KAPC_STATE apcState;
    KeStackAttachProcess(process, &apcState);
    {
        UNICODE_STRING path;
        UNICODE_STRING commandLine;

        status = GetCurrentProcessPathArgs(&path, &commandLine);
        if (NT_SUCCESS(status)) {
            const HANDLE processId = (HANDLE) (ptrdiff_t) processEntry->ProcessId;
            const HANDLE parentProcessId = (HANDLE) (ptrdiff_t) processEntry->ParentProcessId;

            PS_CREATE_NOTIFY_INFO createInfo = { .ParentProcessId = parentProcessId,
                .ImageFileName = &path,
                .CommandLine = &commandLine };

            fort_pstree_notify(/*process=*/NULL, processId, &createInfo);
        }
    }
    KeUnstackDetachProcess(&apcState);

    ObDereferenceObject(process);
}

static void fort_pstree_enum_processes_loop(PSYSTEM_PROCESSES processEntry)
{
    for (;;) {
        const DWORD processId = (DWORD) processEntry->ProcessId;

        const HANDLE processHandle = OpenProcessById(processId);
        if (processHandle != NULL) {
            fort_pstree_attach_process(processEntry, processHandle);

            ZwClose(processHandle);
        }

        if (processEntry->NextEntryOffset == 0)
            break;

        processEntry = (PSYSTEM_PROCESSES) ((PUCHAR) processEntry + processEntry->NextEntryOffset);
    }
}

FORT_API void NTAPI fort_pstree_enum_processes(void)
{
    NTSTATUS status;

    ULONG bufferSize;
    status = ZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &bufferSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
        return;

    bufferSize *= 3; /* for possibly new created processes/threads */

    PVOID buffer = fort_mem_alloc(bufferSize, FORT_PSTREE_POOL_TAG);
    if (buffer == NULL)
        return;

    status = ZwQuerySystemInformation(SystemProcessInformation, buffer, bufferSize, &bufferSize);
    if (NT_SUCCESS(status)) {
        fort_pstree_enum_processes_loop(buffer);
    } else {
        LOG("PsTree: Enum Processes Error: %x\n", status);
    }

    fort_mem_free(buffer, FORT_PSTREE_POOL_TAG);
}
