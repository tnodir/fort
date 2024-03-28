/* Fort Firewall Process Tree */

#include "fortps.h"

#include "fortcb.h"
#include "fortdbg.h"
#include "fortdev.h"
#include "forttrace.h"
#include "fortutl.h"

#define FORT_PSTREE_POOL_TAG 'PwfF'

#define FORT_SVCHOST_PREFIX L"\\svchost\\"
#define FORT_SVCHOST_PREFIX_SIZE                                                                   \
    (sizeof(FORT_SVCHOST_PREFIX) - sizeof(WCHAR)) /* skip terminating zero */

#define FORT_SVCHOST_EXE L"svchost.exe"

#define FORT_PSTREE_NAME_LEN_MAX      120
#define FORT_PSTREE_NAME_LEN_MAX_SIZE (FORT_PSTREE_NAME_LEN_MAX * sizeof(WCHAR))
#define FORT_PSTREE_NAMES_POOL_SIZE   (4 * 1024)

#define FORT_PSNAME_DATA_OFF offsetof(FORT_PSNAME, data)

typedef struct fort_psname
{
    UINT16 refcount;
    UINT16 size;
    WCHAR data[1];
} FORT_PSNAME, *PFORT_PSNAME;

#define FORT_PSNODE_NAME_INHERIT   0x0001
#define FORT_PSNODE_NAME_INHERITED 0x0002
#define FORT_PSNODE_NAME_CUSTOM    0x0004
#define FORT_PSNODE_KILL_PROCESS   0x0008
#define FORT_PSNODE_KILL_CHILD     0x0010
#define FORT_PSNODE_IS_SVCHOST     0x0020

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_psnode
{
    struct fort_psnode *next;
    struct fort_psnode *prev;

    PFORT_PSNAME ps_name; /* tommy_hashdyn_node::data */

    tommy_key_t pid_hash; /* tommy_hashdyn_node::index */

    UINT32 process_id;

    UINT16 volatile flags;
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

NTSTATUS NTAPI ZwQueryInformationProcess(HANDLE processHandle, ULONG processInformationClass,
        PVOID processInformation, ULONG processInformationLength, PULONG returnLength);

#endif

#define FORT_COMMAND_LINE_LEN  512
#define FORT_COMMAND_LINE_SIZE (FORT_COMMAND_LINE_LEN * sizeof(WCHAR))

typedef struct fort_path_buffer
{
    UNICODE_STRING path;
    WCHAR buffer[FORT_CONF_APP_PATH_MAX];
} FORT_PATH_BUFFER, *PFORT_PATH_BUFFER;

typedef struct fort_psinfo_hash
{
    tommy_key_t pid_hash;
    HANDLE processHandle;
    DWORD processId;
    DWORD parentProcessId;

    PCUNICODE_STRING path;
    PCUNICODE_STRING commandLine;
} FORT_PSINFO_HASH, *PFORT_PSINFO_HASH;

typedef const FORT_PSINFO_HASH *PCFORT_PSINFO_HASH;

typedef struct fort_pstree_notify_arg
{
    PEPROCESS process;
    HANDLE processId;
    PPS_CREATE_NOTIFY_INFO createInfo;
} FORT_PSTREE_NOTIFY_ARG, *PFORT_PSTREE_NOTIFY_ARG;

typedef const FORT_PSTREE_NOTIFY_ARG *PCFORT_PSTREE_NOTIFY_ARG;

#define fort_pstree_proc_hash(process_id) tommy_inthash_u32((UINT32) (process_id))

#define fort_pstree_get_proc(ps_tree, index)                                                       \
    ((PFORT_PSNODE) tommy_arrayof_ref(&(ps_tree)->procs, (index)))

inline static BOOL fort_is_system_process(DWORD processId, DWORD parentProcessId)
{
    /* System (sub)processes */
    return (processId == 0 || processId == 4 || parentProcessId == 4);
}

static NTSTATUS GetProcessImageName(HANDLE processHandle, PFORT_PATH_BUFFER pb)
{
    NTSTATUS status;

    ULONG outLength;
    status = ZwQueryInformationProcess(
            processHandle, ProcessImageFileName, pb, sizeof(FORT_PATH_BUFFER), &outLength);

    if (!NT_SUCCESS(status))
        return status;

    if (outLength == 0 || pb->path.Length == 0)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    RtlDowncaseUnicodeString(&pb->path, &pb->path, FALSE);
    pb->buffer[pb->path.Length / sizeof(WCHAR)] = L'\0';

    return STATUS_SUCCESS;
}

static HANDLE OpenProcessById(DWORD processId)
{
    NTSTATUS status;

    PEPROCESS peProcess;
    status = PsLookupProcessByProcessId((HANDLE) (ptrdiff_t) processId, &peProcess);
    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Lookup Process Error: pid=%d %x\n", processId, status);
        return NULL;
    }

    HANDLE processHandle;
    status = ObOpenObjectByPointer(
            peProcess, OBJ_KERNEL_HANDLE, NULL, 0, 0, KernelMode, &processHandle);

    ObDereferenceObject(peProcess);

    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Open Process Object Error: pid=%d %x\n", processId, status);
        return NULL;
    }

    return processHandle;
}

UCHAR fort_pstree_flags_set(PFORT_PSTREE ps_tree, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&ps_tree->flags, flags) : InterlockedAnd8(&ps_tree->flags, ~flags);
}

UCHAR fort_pstree_flags(PFORT_PSTREE ps_tree)
{
    return fort_pstree_flags_set(ps_tree, 0, TRUE);
}

static PFORT_PATH_BUFFER fort_pstree_path_buffer_new(void)
{
    PFORT_PATH_BUFFER pb = fort_mem_alloc(sizeof(FORT_PATH_BUFFER), FORT_PSTREE_POOL_TAG);
    if (pb == NULL)
        return NULL;

    pb->path.Length = 0;
    pb->path.MaximumLength = FORT_CONF_APP_PATH_MAX_SIZE;
    pb->path.Buffer = pb->buffer;

    return pb;
}

static PFORT_PSNAME fort_pstree_name_new(PFORT_PSTREE ps_tree, UINT16 name_size)
{
    PFORT_PSNAME ps_name = fort_pool_malloc(&ps_tree->pool_list,
            FORT_PSNAME_DATA_OFF + name_size + sizeof(WCHAR)); /* include terminating zero */
    if (ps_name != NULL) {
        ps_name->refcount = 1;
        ps_name->size = name_size;
        ps_name->data[name_size / sizeof(WCHAR)] = L'\0';
    }
    return ps_name;
}

static void fort_pstree_name_del(PFORT_PSTREE ps_tree, PFORT_PSNAME ps_name)
{
    if (ps_name == NULL)
        return;

    if (--ps_name->refcount == 0) {
        fort_pool_free(&ps_tree->pool_list, ps_name);
    }
}

static BOOL fort_pstree_svchost_path_check(PCUNICODE_STRING path)
{
    const USHORT svchostSize = sizeof(FORT_SVCHOST_EXE) - sizeof(WCHAR); /* skip terminating zero */

    const USHORT pathLength = path->Length;
    const PCHAR pathBuffer = (PCHAR) path->Buffer;

    PCUNICODE_STRING sysDrivePath = fort_system_drive_path();
    PCUNICODE_STRING sys32Path = fort_system32_path();

    const USHORT sys32DrivePrefixSize = 2 * sizeof(WCHAR); /* C: */
    const USHORT sys32PathSize = sys32Path->Length - sys32DrivePrefixSize;

    /* Check the total path length */
    if (pathLength != sysDrivePath->Length + sys32PathSize + svchostSize)
        return FALSE;

    /* Check the file name */
    if (RtlCompareMemory(pathBuffer + (pathLength - svchostSize), FORT_SVCHOST_EXE, svchostSize)
            != svchostSize)
        return FALSE;

    /* Check the drive */
    if (RtlCompareMemory(pathBuffer, sysDrivePath->Buffer, sysDrivePath->Length)
            != sysDrivePath->Length)
        return FALSE;

    /* Check the path */
    if (RtlCompareMemory(pathBuffer + sysDrivePath->Length,
                (PCHAR) sys32Path->Buffer + sys32DrivePrefixSize, sys32PathSize)
            != sys32PathSize)
        return FALSE;

    return TRUE;
}

static BOOL fort_pstree_svchost_check(PCUNICODE_STRING commandLine, PUNICODE_STRING serviceName)
{
    PWCHAR argp = wcsstr(commandLine->Buffer, L"-s ");
    if (argp == NULL)
        return FALSE;

    argp += (sizeof(L"-s ") - sizeof(WCHAR)) / sizeof(WCHAR); /* skip terminating zero */

    PCWCHAR endp = wcschr(argp, L' ');
    if (endp == NULL) {
        endp = (PCWCHAR) ((PCHAR) commandLine->Buffer + commandLine->Length);
    }

    const USHORT nameLen = (USHORT) ((PCHAR) endp - (PCHAR) argp);
    if (nameLen >= FORT_PSTREE_NAME_LEN_MAX_SIZE)
        return FALSE;

    serviceName->Length = nameLen;
    serviceName->MaximumLength = nameLen;
    serviceName->Buffer = argp;

    return TRUE;
}

static PFORT_PSNAME fort_pstree_create_service_name(
        PFORT_PSTREE ps_tree, PCUNICODE_STRING serviceName)
{
    const USHORT nameLen = serviceName->Length;

    PFORT_PSNAME ps_name = fort_pstree_name_new(ps_tree, FORT_SVCHOST_PREFIX_SIZE + nameLen);

    if (ps_name != NULL) {
        PCHAR data = (PCHAR) &ps_name->data;
        RtlCopyMemory(data, FORT_SVCHOST_PREFIX, FORT_SVCHOST_PREFIX_SIZE);

        UNICODE_STRING nameString;
        nameString.Length = nameLen;
        nameString.MaximumLength = nameLen;
        nameString.Buffer = (PWSTR) (data + FORT_SVCHOST_PREFIX_SIZE);

        /* RtlDowncaseUnicodeString() must be called in <DISPATCH level only! */
        fort_ascii_downcase(&nameString, serviceName);
    }

    return ps_name;
}

static void fort_pstree_proc_set_service_name(PFORT_PSNODE proc, PFORT_PSNAME ps_name)
{
    assert(proc->ps_name == NULL);

    proc->ps_name = ps_name;

    if (ps_name != NULL) {
        /* Service can't inherit parent's name */
        proc->flags |= FORT_PSNODE_NAME_CUSTOM;
    }
}

static void fort_pstree_proc_check_svchost(
        PFORT_PSTREE ps_tree, PCFORT_PSINFO_HASH psi, PFORT_PSNODE proc)
{
    if (psi->path == NULL || psi->commandLine == NULL)
        return;

    if (!fort_pstree_svchost_path_check(psi->path))
        return;

    proc->flags |= FORT_PSNODE_IS_SVCHOST;

    UNICODE_STRING serviceName;
    if (!fort_pstree_svchost_check(psi->commandLine, &serviceName))
        return;

    PFORT_PSNAME ps_name = fort_pstree_create_service_name(ps_tree, &serviceName);

    fort_pstree_proc_set_service_name(proc, ps_name);
}

static PFORT_PSNODE fort_pstree_proc_new(PFORT_PSTREE ps_tree, tommy_key_t pid_hash)
{
    tommy_hashdyn_node *proc_node = tommy_list_tail(&ps_tree->free_procs);

    if (proc_node != NULL) {
        tommy_list_remove_existing(&ps_tree->free_procs, proc_node);
    } else {
        tommy_arrayof *procs = &ps_tree->procs;
        const UINT16 index = ps_tree->procs_n;

        tommy_arrayof_grow(procs, index + 1);

        proc_node = tommy_arrayof_ref(procs, index);
    }

    tommy_hashdyn_insert(&ps_tree->procs_map, proc_node, /*ps_name=*/NULL, pid_hash);

    ++ps_tree->procs_n;

    return (PFORT_PSNODE) proc_node;
}

static void fort_pstree_proc_del(PFORT_PSTREE ps_tree, PFORT_PSNODE proc)
{
    --ps_tree->procs_n;

    /* Delete from pool */
    fort_pstree_name_del(ps_tree, proc->ps_name);

    proc->ps_name = NULL;
    proc->process_id = 0;

    /* Delete from procs map */
    tommy_hashdyn_remove_existing(&ps_tree->procs_map, (tommy_hashdyn_node *) proc);

    tommy_list_insert_tail_check(&ps_tree->free_procs, (tommy_node *) proc);
}

static PFORT_PSNODE fort_pstree_find_proc_hash(
        PFORT_PSTREE ps_tree, DWORD processId, tommy_key_t pid_hash)
{
    PFORT_PSNODE node = (PFORT_PSNODE) tommy_hashdyn_bucket(&ps_tree->procs_map, pid_hash);

    while (node != NULL) {
        if (node->process_id == processId)
            return node;

        node = node->next;
    }

    return NULL;
}

static PFORT_PSNODE fort_pstree_find_proc(PFORT_PSTREE ps_tree, DWORD processId)
{
    if (processId == 0)
        return NULL;

    const tommy_key_t pid_hash = fort_pstree_proc_hash(processId);

    return fort_pstree_find_proc_hash(ps_tree, processId, pid_hash);
}

inline static void fort_pstree_proc_set_name(
        PFORT_PSTREE ps_tree, PFORT_PSNODE proc, const PVOID path_buf, UINT16 path_len)
{
    PFORT_PSNAME ps_name = fort_pstree_name_new(ps_tree, path_len);
    if (ps_name == NULL)
        return;

    RtlCopyMemory(ps_name->data, path_buf, path_len);

    proc->ps_name = ps_name;
}

inline static void fort_pstree_check_proc_app_flags(PFORT_PSTREE ps_tree, PFORT_PSNODE proc,
        const PVOID path_buf, UINT16 path_len, FORT_APP_FLAGS app_flags)
{
    const UINT16 kill_flags = (app_flags.kill_process ? FORT_PSNODE_KILL_PROCESS : 0)
            | (app_flags.kill_child ? FORT_PSNODE_KILL_CHILD : 0);
    proc->flags |= kill_flags;

    if (kill_flags == 0 && app_flags.apply_child) {
        const BOOL has_ps_name = (proc->ps_name != NULL);
        if (!has_ps_name) {
            fort_pstree_proc_set_name(ps_tree, proc, path_buf, path_len);
        }

        proc->flags |= FORT_PSNODE_NAME_INHERIT;
    }
}

inline static void fort_pstree_check_proc_conf(
        PFORT_PSTREE ps_tree, PFORT_CONF_REF conf_ref, PFORT_PSNODE proc, PCUNICODE_STRING path)
{
    const BOOL has_ps_name = (proc->ps_name != NULL);
    const PVOID path_buf = has_ps_name ? proc->ps_name->data : path->Buffer;
    const UINT16 path_len = has_ps_name ? proc->ps_name->size : path->Length;

    const PFORT_CONF conf = &conf_ref->conf;

    const FORT_APP_DATA app_data = conf->proc_wild
            ? fort_conf_app_find(conf, path_buf, path_len, fort_conf_exe_find, conf_ref)
            : fort_conf_exe_find(conf, conf_ref, path_buf, path_len);

    if (app_data.flags.v != 0) {
        fort_pstree_check_proc_app_flags(ps_tree, proc, path_buf, path_len, app_data.flags);
    }
}

inline static BOOL fort_pstree_check_proc_inherited(
        PFORT_PSTREE ps_tree, PFORT_PSNODE proc, DWORD parentProcessId)
{
    if (proc->ps_name != NULL)
        return FALSE;

    PFORT_PSNODE parent = fort_pstree_find_proc(ps_tree, parentProcessId);
    if (parent == NULL)
        return FALSE;

    if ((parent->flags & (FORT_PSNODE_NAME_INHERIT | FORT_PSNODE_NAME_INHERITED)) == 0)
        return FALSE;

    PFORT_PSNAME ps_name = parent->ps_name;
    assert(ps_name != NULL);

    ++ps_name->refcount;
    proc->ps_name = ps_name;

    proc->flags |= FORT_PSNODE_NAME_INHERITED;

    return TRUE;
}

static void fort_pstree_check_proc_inheritance(
        PFORT_PSTREE ps_tree, PCFORT_PSINFO_HASH psi, PFORT_PSNODE proc)
{
    if (psi->path == NULL)
        return;

    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;

    PFORT_CONF_REF conf_ref = fort_conf_ref_take(device_conf);
    if (conf_ref == NULL)
        return;

    if (!fort_pstree_check_proc_inherited(ps_tree, proc, psi->parentProcessId)) {
        fort_pstree_check_proc_conf(ps_tree, conf_ref, proc, psi->path);
    }

    fort_conf_ref_put(device_conf, conf_ref);
}

static PFORT_PSNODE fort_pstree_handle_new_proc(PFORT_PSTREE ps_tree, PCFORT_PSINFO_HASH psi)
{
    PFORT_PSNODE proc = fort_pstree_proc_new(ps_tree, psi->pid_hash);
    if (proc == NULL)
        return NULL;

    proc->process_id = psi->processId;
    proc->flags = 0;

    fort_pstree_proc_check_svchost(ps_tree, psi, proc);

    fort_pstree_check_proc_inheritance(ps_tree, psi, proc);

    return proc;
}

inline static BOOL fort_pstree_check_kill_proc(
        PFORT_PSNODE proc, PPS_CREATE_NOTIFY_INFO createInfo, UINT16 flags)
{
    if (proc != NULL && (proc->flags & flags) != 0) {
        createInfo->CreationStatus = STATUS_ACCESS_DENIED;
        /* later arrives notification about the process's close event */
        return TRUE;
    }
    return FALSE;
}

inline static PFORT_PSNODE fort_pstree_handle_opened_proc(
        PFORT_PSTREE ps_tree, PFORT_PSINFO_HASH psi, PFORT_PATH_BUFFER pb)
{
    /* GetProcessImageName() must be called in PASSIVE level only! */
    const NTSTATUS status = GetProcessImageName(psi->processHandle, pb);
    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Image Name Error: %x\n", status);
        return NULL;
    }

    PFORT_PSNODE proc;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        proc = fort_pstree_handle_new_proc(ps_tree, psi);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return proc;
}

static PFORT_PSNODE fort_pstree_handle_created_proc(PFORT_PSTREE ps_tree, PFORT_PSINFO_HASH psi)
{
    const HANDLE processHandle = OpenProcessById(psi->processId);
    if (processHandle == NULL)
        return NULL;

    PFORT_PSNODE proc = NULL;

    PFORT_PATH_BUFFER pb = fort_pstree_path_buffer_new();
    if (pb != NULL) {
        psi->processHandle = processHandle;
        psi->path = &pb->path;

        proc = fort_pstree_handle_opened_proc(ps_tree, psi, pb);

        fort_mem_free(pb, FORT_PSTREE_POOL_TAG);
    }

    ZwClose(processHandle);

    return proc;
}

inline static void fort_pstree_notify_process_created(
        PFORT_PSTREE ps_tree, PPS_CREATE_NOTIFY_INFO createInfo, PFORT_PSINFO_HASH psi)
{
    if (createInfo->ImageFileName == NULL || createInfo->CommandLine == NULL)
        return;

    if (fort_is_system_process(psi->processId, psi->parentProcessId))
        return; /* skip System (sub)processes */

    PFORT_PSNODE proc = fort_pstree_handle_created_proc(ps_tree, psi);

    fort_pstree_check_kill_proc(proc, createInfo, FORT_PSNODE_KILL_PROCESS);
}

inline static BOOL fort_pstree_notify_process_prepare(
        PFORT_PSTREE ps_tree, PPS_CREATE_NOTIFY_INFO createInfo, PFORT_PSINFO_HASH psi)
{
    BOOL res = TRUE;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);

    PFORT_PSNODE proc = fort_pstree_find_proc_hash(ps_tree, psi->processId, psi->pid_hash);
    if (proc != NULL) {
        fort_pstree_proc_del(ps_tree, proc);
    }

    /* Check parent process */
    if (createInfo != NULL && !fort_is_system_process(psi->parentProcessId, -1)) {
        const tommy_key_t ppid_hash = fort_pstree_proc_hash(psi->parentProcessId);
        PFORT_PSNODE parentProc =
                fort_pstree_find_proc_hash(ps_tree, psi->parentProcessId, ppid_hash);

        res = !fort_pstree_check_kill_proc(parentProc, createInfo, FORT_PSNODE_KILL_CHILD);
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return res;
}

inline static void fort_pstree_notify_process(PFORT_PSTREE ps_tree, PCFORT_PSTREE_NOTIFY_ARG pna)
{
    PPS_CREATE_NOTIFY_INFO createInfo = pna->createInfo;
    const DWORD parentProcessId =
            (DWORD) (ptrdiff_t) (createInfo != NULL ? createInfo->ParentProcessId : 0);
    const DWORD processId = (DWORD) (ptrdiff_t) pna->processId;

    FORT_PSINFO_HASH psi = {
        .pid_hash = fort_pstree_proc_hash(processId),
        .processId = processId,
        .parentProcessId = parentProcessId,

        .commandLine = (createInfo != NULL ? createInfo->CommandLine : NULL),
    };

#ifdef FORT_DEBUG
    if (createInfo == NULL) {
        LOG("PsTree: CLOSED pid=%d\n", processId);
    } else {
        LOG("PsTree: NEW pid=%d ppid=%d IMG=[%wZ] CMD=[%wZ]\n", processId, parentProcessId,
                createInfo->ImageFileName, createInfo->CommandLine);
    }
#endif

    if (!fort_pstree_notify_process_prepare(ps_tree, createInfo, &psi))
        return;

    if (createInfo != NULL) {
        fort_pstree_notify_process_created(ps_tree, createInfo, &psi);
    }
}

static NTSTATUS fort_pstree_notify_expand(PVOID param)
{
    PCFORT_PSTREE_NOTIFY_ARG pna = param;

    PFORT_PSTREE ps_tree = &fort_device()->ps_tree;

    fort_pstree_notify_process(ps_tree, pna);

    return STATUS_SUCCESS;
}

static void NTAPI fort_pstree_notify(
        PEPROCESS process, HANDLE processId, PPS_CREATE_NOTIFY_INFO createInfo)
{
    FORT_PSTREE_NOTIFY_ARG pna = {
        .process = process,
        .processId = processId,
        .createInfo = createInfo,
    };

    fort_expand_stack(&fort_pstree_notify_expand, &pna);
}

static void fort_pstree_update(PFORT_PSTREE ps_tree, BOOL active)
{
    const UCHAR flags = fort_pstree_flags_set(ps_tree, FORT_PSTREE_ACTIVE, active);
    const BOOL was_active = (flags & FORT_PSTREE_ACTIVE) != 0;

    if (was_active == active)
        return;

    const NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(
            FORT_CALLBACK(FORT_CALLBACK_PSTREE_NOTIFY, PCREATE_PROCESS_NOTIFY_ROUTINE_EX,
                    &fort_pstree_notify),
            /*remove=*/!active);

    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Update Error: %x\n", status);
        TRACE(FORT_PSTREE_UPDATE_ERROR, status, 0, 0);
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

    fort_pstree_update(ps_tree, /*active=*/TRUE); /* Start process monitor */
}

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree)
{
    fort_pstree_update(ps_tree, /*active=*/FALSE); /* Stop process monitor */

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        fort_pool_done(&ps_tree->pool_list);

        tommy_arrayof_done(&ps_tree->procs);
        tommy_hashdyn_done(&ps_tree->procs_map);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

inline static BOOL fort_pstree_enum_process_exists(
        PFORT_PSTREE ps_tree, DWORD processId, tommy_key_t pid_hash)
{
    PFORT_PSNODE proc;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        proc = fort_pstree_find_proc_hash(ps_tree, processId, pid_hash);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return (proc != NULL);
}

inline static void fort_pstree_enum_process(PFORT_PSTREE ps_tree, PSYSTEM_PROCESSES processEntry)
{
    const DWORD processId = (DWORD) processEntry->ProcessId;
    const DWORD parentProcessId = (DWORD) processEntry->ParentProcessId;

    if (fort_is_system_process(processId, parentProcessId))
        return; /* skip System (sub)processes */

    const tommy_key_t pid_hash = fort_pstree_proc_hash(processId);

    if (fort_pstree_enum_process_exists(ps_tree, processId, pid_hash))
        return;

    FORT_PSINFO_HASH psi = {
        .pid_hash = pid_hash,
        .processId = processId,
        .parentProcessId = parentProcessId,
    };

    fort_pstree_handle_created_proc(ps_tree, &psi);
}

inline static void fort_pstree_enum_processes_loop(
        PFORT_PSTREE ps_tree, PSYSTEM_PROCESSES processEntry)
{
    for (;;) {
        fort_pstree_enum_process(ps_tree, processEntry);

        const ULONG nextEntryOffset = processEntry->NextEntryOffset;
        if (nextEntryOffset == 0)
            break;

        processEntry = (PSYSTEM_PROCESSES) ((PUCHAR) processEntry + nextEntryOffset);
    }
}

FORT_API void fort_pstree_enum_processes(PFORT_PSTREE ps_tree)
{
    NTSTATUS status;

    ULONG bufferSize;
    status = ZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &bufferSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
        return;

    bufferSize *= 2; /* for possibly new created processes/threads */
    bufferSize = FORT_ALIGN_SIZE(bufferSize, sizeof(PVOID));

    PVOID buffer = fort_mem_alloc(bufferSize, FORT_PSTREE_POOL_TAG);
    if (buffer == NULL)
        return;

    status = ZwQuerySystemInformation(SystemProcessInformation, buffer, bufferSize, NULL);
    if (NT_SUCCESS(status)) {
        fort_pstree_enum_processes_loop(&fort_device()->ps_tree, buffer);
    } else {
        LOG("PsTree: Enum Processes Error: %x\n", status);
        TRACE(FORT_PSTREE_ENUM_PROCESSES_ERROR, status, 0, 0);
    }

    fort_mem_free(buffer, FORT_PSTREE_POOL_TAG);
}

static BOOL fort_pstree_get_proc_name_locked(PFORT_PSTREE ps_tree, DWORD processId,
        PUNICODE_STRING path, BOOL *isSvcHost, BOOL *inherited)
{
    PFORT_PSNODE proc = fort_pstree_find_proc(ps_tree, processId);
    if (proc == NULL)
        return FALSE;

    const UINT16 procFlags = proc->flags;
    *isSvcHost = (procFlags & FORT_PSNODE_IS_SVCHOST) != 0;

    PFORT_PSNAME ps_name = proc->ps_name;
    if (ps_name == NULL)
        return FALSE;

    if ((procFlags & (FORT_PSNODE_NAME_INHERIT | FORT_PSNODE_NAME_CUSTOM))
            == FORT_PSNODE_NAME_INHERIT)
        return FALSE;

    path->Length = ps_name->size;
    path->MaximumLength = ps_name->size;
    path->Buffer = ps_name->data;

    *inherited = (procFlags & FORT_PSNODE_NAME_INHERITED) != 0;

    return TRUE;
}

FORT_API BOOL fort_pstree_get_proc_name(PFORT_PSTREE ps_tree, DWORD processId, PUNICODE_STRING path,
        BOOL *isSvcHost, BOOL *inherited)
{
    BOOL res;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        res = fort_pstree_get_proc_name_locked(ps_tree, processId, path, isSvcHost, inherited);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return res;
}

inline static void fort_pstree_update_service_proc(
        PFORT_PSTREE ps_tree, PCUNICODE_STRING serviceName, DWORD processId)
{
    const tommy_key_t pid_hash = fort_pstree_proc_hash(processId);

    PFORT_PSNODE proc = fort_pstree_find_proc_hash(ps_tree, processId, pid_hash);
    if (proc == NULL) {
        proc = fort_pstree_proc_new(ps_tree, pid_hash);

        proc->process_id = processId;
        proc->flags = 0;
    }

    if (proc != NULL && proc->ps_name == NULL) {
        PFORT_PSNAME ps_name = fort_pstree_create_service_name(ps_tree, serviceName);

        fort_pstree_proc_set_service_name(proc, ps_name);
    }
}

static int fort_pstree_update_service(
        PFORT_PSTREE ps_tree, const PFORT_SERVICE_INFO service, const PCHAR end_data)
{
    if ((PCHAR) service + FORT_SERVICE_INFO_NAME_OFF > end_data)
        return 0;

    UNICODE_STRING serviceName;
    serviceName.Length = service->name_len;
    serviceName.MaximumLength = serviceName.Length;
    serviceName.Buffer = service->name;

    if ((PCHAR) service + FORT_SERVICE_INFO_NAME_OFF + serviceName.Length > end_data)
        return 0;

    fort_pstree_update_service_proc(ps_tree, &serviceName, service->process_id);

    return FORT_SERVICE_INFO_NAME_OFF + FORT_CONF_STR_DATA_SIZE(serviceName.Length);
}

FORT_API void fort_pstree_update_services(
        PFORT_PSTREE ps_tree, const PFORT_SERVICE_INFO_LIST services, ULONG data_len)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        PCHAR data = (PCHAR) services->data;
        const PCHAR end_data = data + data_len;

        UINT16 n = services->services_n;
        while (n-- > 0) {
            const int size =
                    fort_pstree_update_service(ps_tree, (PFORT_SERVICE_INFO) data, end_data);
            if (size == 0)
                break;

            data += size;
        }
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}
