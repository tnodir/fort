#ifndef UM_NTDDK_H
#define UM_NTDDK_H

#include "um_wdm.h"

#define KERNEL_STACK_SIZE 12288

typedef struct _KPROCESS *PEPROCESS;

typedef struct _PS_CREATE_NOTIFY_INFO
{
    SIZE_T Size;
    union {
        ULONG Flags;
        struct
        {
            ULONG FileOpenNameAvailable : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG Reserved : 30;
        };
    };
    HANDLE ParentProcessId;
    CLIENT_ID CreatingThreadId;
    struct _FILE_OBJECT *FileObject;
    PCUNICODE_STRING ImageFileName;
    PCUNICODE_STRING CommandLine;
    NTSTATUS CreationStatus;
} PS_CREATE_NOTIFY_INFO, *PPS_CREATE_NOTIFY_INFO;

typedef VOID (*PCREATE_PROCESS_NOTIFY_ROUTINE_EX)(
        PEPROCESS process, HANDLE processId, PPS_CREATE_NOTIFY_INFO createInfo);

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS IoQueryFullDriverPath(PDRIVER_OBJECT driverObject, PUNICODE_STRING fullPath);

FORT_API NTSTATUS PsSetCreateProcessNotifyRoutineEx(
        PCREATE_PROCESS_NOTIFY_ROUTINE_EX notifyRoutine, BOOLEAN remove);

typedef void(NTAPI EXPAND_STACK_CALLOUT)(PVOID parameter);
typedef EXPAND_STACK_CALLOUT *PEXPAND_STACK_CALLOUT;

FORT_API NTSTATUS KeExpandKernelStackAndCallout(
        PEXPAND_STACK_CALLOUT callout, PVOID parameter, SIZE_T size);

FORT_API LONG KeSetBasePriorityThread(PVOID threadObject, LONG increment);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_NTDDK_H
