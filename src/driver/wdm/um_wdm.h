#ifndef UM_WDM_H
#define UM_WDM_H

#include "../common/common.h"

#pragma warning(push)
#pragma warning(disable : 4005) // suppress warning: C4005: macro redefinition
#include <ifdef.h>
#include <ntstatus.h>
#include <winternl.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#pragma warning(pop)

#if defined(__cplusplus)
extern "C" {
#endif

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

typedef LONG *PCALLBACK_OBJECT;

typedef UCHAR KIRQL, *PKIRQL;

typedef LONG KDPC, *PKDPC, *PRKDPC;

//
// Define the major function codes for IRPs.
//
#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b

//
// POWER minor function codes
//
#define IRP_MN_WAIT_WAKE      0x00
#define IRP_MN_POWER_SEQUENCE 0x01
#define IRP_MN_SET_POWER      0x02
#define IRP_MN_QUERY_POWER    0x03

//
// Define Device Object (DO) flags
//
#define DO_VERIFY_VOLUME         0x00000002
#define DO_BUFFERED_IO           0x00000004
#define DO_EXCLUSIVE             0x00000008
#define DO_DIRECT_IO             0x00000010
#define DO_MAP_IO_BUFFER         0x00000020
#define DO_DEVICE_INITIALIZING   0x00000080
#define DO_SHUTDOWN_REGISTERED   0x00000800
#define DO_BUS_ENUMERATED_DEVICE 0x00001000
#define DO_POWER_PAGABLE         0x00002000
#define DO_POWER_INRUSH          0x00004000
#define DO_DEVICE_TO_BE_RESET    0x04000000
#define DO_DAX_VOLUME            0x10000000

typedef CCHAR KPROCESSOR_MODE;
typedef enum { KernelMode, UserMode, MaximumMode } MODE;

#if defined(_WIN64)
#    define POINTER_ALIGNMENT DECLSPEC_ALIGN(8)
#else
#    define POINTER_ALIGNMENT
#endif

typedef union _POWER_STATE {
    SYSTEM_POWER_STATE SystemState;
    DEVICE_POWER_STATE DeviceState;
} POWER_STATE, *PPOWER_STATE;

typedef enum _POWER_STATE_TYPE {
    SystemPowerState = 0,
    DevicePowerState
} POWER_STATE_TYPE,
        *PPOWER_STATE_TYPE;

typedef struct _SYSTEM_POWER_STATE_CONTEXT
{
    union {
        struct
        {
            ULONG Reserved1 : 8;
            ULONG TargetSystemState : 4;
            ULONG EffectiveSystemState : 4;
            ULONG CurrentSystemState : 4;
            ULONG IgnoreHibernationPath : 1;
            ULONG PseudoTransition : 1;
            ULONG KernelSoftReboot : 1;
            ULONG DirectedDripsTransition : 1;
            ULONG Reserved2 : 8;
        } DUMMYSTRUCTNAME;

        ULONG ContextAsUlong;
    } DUMMYUNIONNAME;
} SYSTEM_POWER_STATE_CONTEXT, *PSYSTEM_POWER_STATE_CONTEXT;

typedef struct
{
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;

    union {
        struct
        {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            ULONG IoControlCode;
        } DeviceIoControl;

        struct
        {
            union {
                ULONG SystemContext;
                SYSTEM_POWER_STATE_CONTEXT SystemPowerStateContext;
            };
            POWER_STATE_TYPE POINTER_ALIGNMENT Type;
            POWER_STATE POINTER_ALIGNMENT State;
            POWER_ACTION POINTER_ALIGNMENT ShutdownType;
        } Power;
    } Parameters;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

typedef struct
{
    struct
    {
        NTSTATUS Status;
        ULONG_PTR Information;
    } IoStatus;
    union {
        PVOID SystemBuffer;
    } AssociatedIrp;
    KIRQL CancelIrql;
} IRP, *PIRP;

typedef struct _ACCESS_STATE *PACCESS_STATE;
typedef struct _KPROCESS *PKPROCESS, *PRKPROCESS, *PEPROCESS;
typedef struct _OBJECT_TYPE *POBJECT_TYPE;

typedef struct _OBJECT_HANDLE_INFORMATION
{
    ULONG HandleAttributes;
    ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

struct _DRIVER_OBJECT;

typedef struct _DEVICE_OBJECT
{
    USHORT Size;
    struct _DRIVER_OBJECT *DriverObject;
    struct _DEVICE_OBJECT *NextDevice;
    struct _DEVICE_OBJECT *AttachedDevice;
    struct _IRP *CurrentIrp;
    ULONG Flags;
    ULONG Characteristics;
    PVOID DeviceExtension;
    DEVICE_TYPE DeviceType;
    CCHAR StackSize;
    ULONG AlignmentRequirement;
    KDPC Dpc;
} DEVICE_OBJECT, *PDEVICE_OBJECT;

typedef VOID DRIVER_UNLOAD(struct _DRIVER_OBJECT *driver);
typedef DRIVER_UNLOAD *PDRIVER_UNLOAD;

typedef NTSTATUS DRIVER_DISPATCH(PDEVICE_OBJECT device, PIRP irp);
typedef DRIVER_DISPATCH *PDRIVER_DISPATCH;

typedef struct _DRIVER_OBJECT
{
    PDEVICE_OBJECT DeviceObject;
    ULONG Flags;

    UNICODE_STRING DriverName;

    PDRIVER_UNLOAD DriverUnload;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} DRIVER_OBJECT, *PDRIVER_OBJECT;

typedef void KDEFERRED_ROUTINE(PKDPC dpc, PVOID context, PVOID sysArg1, PVOID sysArg2);
typedef KDEFERRED_ROUTINE *PKDEFERRED_ROUTINE;

typedef void DRIVER_CANCEL(PDEVICE_OBJECT device, PIRP irp);
typedef DRIVER_CANCEL *PDRIVER_CANCEL;

typedef VOID CALLBACK_FUNCTION(PVOID context, PVOID arg1, PVOID arg2);
typedef CALLBACK_FUNCTION *PCALLBACK_FUNCTION;

typedef LONG KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;
typedef volatile LONG EX_SPIN_LOCK, *PEX_SPIN_LOCK;

typedef struct _EX_RUNDOWN_REF
{
    union {
        __volatile ULONG_PTR Count;
        __volatile PVOID Ptr;
    };
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

typedef LONG KEVENT, *PKEVENT, *PRKEVENT;

typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent,
} EVENT_TYPE;

typedef enum _KWAIT_REASON {
    Executive,
} KWAIT_REASON;

typedef LONG KTIMER, *PKTIMER;

typedef LONG *PIO_WORKITEM;
typedef VOID IO_WORKITEM_ROUTINE(PDEVICE_OBJECT DeviceObject, PVOID Context);
typedef IO_WORKITEM_ROUTINE *PIO_WORKITEM_ROUTINE;
typedef VOID IO_WORKITEM_ROUTINE_EX(PVOID IoObject, PVOID Context, PIO_WORKITEM IoWorkItem);
typedef IO_WORKITEM_ROUTINE_EX *PIO_WORKITEM_ROUTINE_EX;

typedef VOID KSTART_ROUTINE(PVOID startContext);
typedef KSTART_ROUTINE *PKSTART_ROUTINE;

typedef struct
{
    short Year; // range [1601...]
    short Month; // range [1..12]
    short Day; // range [1..31]
    short Hour; // range [0..23]
    short Minute; // range [0..59]
    short Second; // range [0..59]
    short Milliseconds; // range [0..999]
    short Weekday; // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS, *PTIME_FIELDS;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    _Field_size_bytes_(DataLength) UCHAR Data[1]; // Variable size
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    KeyValueLayerInformation,
    MaxKeyValueInfoClass // MaxKeyValueInfoClass should always be the last enum
} KEY_VALUE_INFORMATION_CLASS;

// Collision with winternl.h
// typedef enum _FILE_INFORMATION_CLASS {
#define FileBasicInformation    4
#define FileStandardInformation 5

typedef struct _FILE_STANDARD_INFORMATION
{
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef NTSTATUS DRIVER_INITIALIZE(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath);
typedef DRIVER_INITIALIZE *PDRIVER_INITIALIZE;

#define DPFLTR_IHVNETWORK_ID 0
#define DPFLTR_ERROR_LEVEL   0
FORT_API ULONG DbgPrintEx(ULONG componentId, ULONG level, PCSTR format, ...);

#define NT_ASSERT(cond) assert((cond))

#define NtCurrentProcess() ((HANDLE) (LONG_PTR) -1)
#define ZwCurrentProcess() NtCurrentProcess()

FORT_API PEPROCESS IoGetCurrentProcess(void);

#define NonPagedPool        0
#define NonPagedPoolExecute NonPagedPool
#define NonPagedPoolNx      512
FORT_API PVOID ExAllocatePoolWithTag(DWORD type, SIZE_T size, ULONG tag);
FORT_API void ExFreePoolWithTag(PVOID p, ULONG tag);
FORT_API PVOID ExAllocatePool(DWORD type, SIZE_T size);
FORT_API void ExFreePool(PVOID p);

typedef ULONG64 POOL_FLAGS;
#define POOL_FLAG_UNINITIALIZED     0x0000000000000002UI64 // Don't zero-initialize allocation
#define POOL_FLAG_NON_PAGED         0x0000000000000040UI64 // Non paged pool NX
#define POOL_FLAG_NON_PAGED_EXECUTE 0x0000000000000080UI64 // Non paged pool executable
#define POOL_FLAG_PAGED             0x0000000000000100UI64 // Paged pool
FORT_API PVOID ExAllocatePool2(POOL_FLAGS flags, SIZE_T size, ULONG tag);

FORT_API PIO_STACK_LOCATION IoGetCurrentIrpStackLocation(PIRP irp);
FORT_API void IoMarkIrpPending(PIRP irp);
FORT_API PDRIVER_CANCEL IoSetCancelRoutine(PIRP irp, PDRIVER_CANCEL routine);

FORT_API void KeInitializeDpc(PRKDPC dpc, PKDEFERRED_ROUTINE routine, PVOID context);
FORT_API void KeFlushQueuedDpcs(void);

#define DrvRtPoolNxOptIn 0x00000001
FORT_API VOID ExInitializeDriverRuntime(ULONG runtimeFlags);

FORT_API NTSTATUS IoCreateDevice(PDRIVER_OBJECT driver, ULONG extensionSize, PUNICODE_STRING name,
        DEVICE_TYPE type, ULONG characteristics, BOOLEAN exclusive, PDEVICE_OBJECT *device);
FORT_API VOID IoDeleteDevice(PDEVICE_OBJECT device);

FORT_API NTSTATUS IoCreateSymbolicLink(PUNICODE_STRING symbolicLink, PUNICODE_STRING deviceName);
FORT_API NTSTATUS IoDeleteSymbolicLink(PUNICODE_STRING symbolicLink);

FORT_API NTSTATUS KeDelayExecutionThread(
        KPROCESSOR_MODE waitMode, BOOLEAN alertable, PLARGE_INTEGER interval);

#define PO_CB_SYSTEM_STATE_LOCK 3
FORT_API NTSTATUS ExCreateCallback(
        PCALLBACK_OBJECT *callback, POBJECT_ATTRIBUTES attr, BOOLEAN create, BOOLEAN multiple);
FORT_API PVOID ExRegisterCallback(
        PCALLBACK_OBJECT callback, PCALLBACK_FUNCTION function, PVOID context);
FORT_API VOID ExUnregisterCallback(PVOID callbackReg);

FORT_API LONG_PTR ObDereferenceObject(PVOID object);

FORT_API void KeInitializeSpinLock(PKSPIN_LOCK lock);
FORT_API void KeAcquireInStackQueuedSpinLock(PKSPIN_LOCK lock, PKLOCK_QUEUE_HANDLE handle);
FORT_API void KeReleaseInStackQueuedSpinLock(PKLOCK_QUEUE_HANDLE handle);
FORT_API void KeAcquireInStackQueuedSpinLockAtDpcLevel(
        PKSPIN_LOCK lock, PKLOCK_QUEUE_HANDLE handle);
FORT_API void KeReleaseInStackQueuedSpinLockFromDpcLevel(PKLOCK_QUEUE_HANDLE handle);

FORT_API void IoAcquireCancelSpinLock(PKIRQL irql);
FORT_API void IoReleaseCancelSpinLock(KIRQL irql);

FORT_API KIRQL ExAcquireSpinLockShared(PEX_SPIN_LOCK lock);
FORT_API KIRQL ExAcquireSpinLockExclusive(PEX_SPIN_LOCK lock);
FORT_API void ExReleaseSpinLockShared(PEX_SPIN_LOCK lock, KIRQL oldIrql);
FORT_API void ExReleaseSpinLockExclusive(PEX_SPIN_LOCK lock, KIRQL oldIrql);

FORT_API KIRQL KeGetCurrentIrql(void);

#define IO_NO_INCREMENT 0
FORT_API void IoCompleteRequest(PIRP irp, CCHAR priorityBoost);

FORT_API void KeInitializeTimer(PKTIMER timer);
FORT_API BOOLEAN KeCancelTimer(PKTIMER timer);
FORT_API BOOLEAN KeSetCoalescableTimer(
        PKTIMER timer, LARGE_INTEGER dueTime, ULONG period, ULONG tolerableDelay, PKDPC dpc);

FORT_API void ExInitializeRundownProtection(PEX_RUNDOWN_REF runRef);
FORT_API BOOLEAN ExAcquireRundownProtection(PEX_RUNDOWN_REF runRef);
FORT_API void ExReleaseRundownProtection(PEX_RUNDOWN_REF runRef);

FORT_API void KeInitializeEvent(PRKEVENT event, EVENT_TYPE type, BOOLEAN state);
FORT_API void KeClearEvent(PRKEVENT event);
FORT_API LONG KeSetEvent(PRKEVENT event, KPRIORITY increment, BOOLEAN wait);

FORT_API NTSTATUS KeWaitForSingleObject(PVOID object, KWAIT_REASON waitReason,
        KPROCESSOR_MODE waitMode, BOOLEAN alertable, PLARGE_INTEGER timeout);

FORT_API PIO_WORKITEM IoAllocateWorkItem(PDEVICE_OBJECT device);
FORT_API void IoFreeWorkItem(PIO_WORKITEM workItem);

#define DelayedWorkQueue 0
FORT_API void IoQueueWorkItem(
        PIO_WORKITEM workItem, PIO_WORKITEM_ROUTINE routine, int queueType, PVOID context);
FORT_API void IoQueueWorkItemEx(
        PIO_WORKITEM workItem, PIO_WORKITEM_ROUTINE_EX routine, int queueType, PVOID context);

FORT_API NTSTATUS PsCreateSystemThread(PHANDLE threadHandle, ULONG desiredAccess,
        POBJECT_ATTRIBUTES objectAttributes, HANDLE processHandle, PVOID clientId,
        PKSTART_ROUTINE startRoutine, PVOID startContext);

FORT_API LARGE_INTEGER KeQueryPerformanceCounter(PLARGE_INTEGER performanceFrequency);

FORT_API void KeQuerySystemTime(PLARGE_INTEGER time);
FORT_API void ExSystemTimeToLocalTime(PLARGE_INTEGER systemTime, PLARGE_INTEGER localTime);
FORT_API void RtlTimeToTimeFields(PLARGE_INTEGER time, PTIME_FIELDS timeFields);

FORT_API NTSTATUS RtlGetVersion(PRTL_OSVERSIONINFOW versionInformation);

FORT_API NTSTATUS ZwOpenKey(
        PHANDLE keyHandle, ACCESS_MASK desiredAccess, POBJECT_ATTRIBUTES objectAttributes);
FORT_API NTSTATUS ZwClose(HANDLE handle);

FORT_API NTSTATUS ZwQueryValueKey(HANDLE keyHandle, PUNICODE_STRING valueName,
        KEY_VALUE_INFORMATION_CLASS keyValueInformationClass, PVOID keyValueInformation,
        ULONG length, PULONG resultLength);

FORT_API NTSTATUS ZwOpenFile(PHANDLE fileHandle, ACCESS_MASK desiredAccess,
        POBJECT_ATTRIBUTES objectAttributes, PIO_STATUS_BLOCK ioStatusBlock, ULONG shareAccess,
        ULONG openOptions);
FORT_API NTSTATUS ZwQueryInformationFile(HANDLE fileHandle, PIO_STATUS_BLOCK ioStatusBlock,
        PVOID fileInformation, ULONG length, FILE_INFORMATION_CLASS fileInformationClass);
FORT_API NTSTATUS ZwReadFile(HANDLE fileHandle, HANDLE event, PIO_APC_ROUTINE apcRoutine,
        PVOID apcContext, PIO_STATUS_BLOCK ioStatusBlock, PVOID buffer, ULONG length,
        PLARGE_INTEGER byteOffset, PULONG key);

#define DIRECTORY_QUERY     (0x0001)
#define SYMBOLIC_LINK_QUERY (0x0001)

FORT_API NTSTATUS ZwOpenDirectoryObject(
        PHANDLE directoryHandle, ACCESS_MASK desiredAccess, POBJECT_ATTRIBUTES objectAttributes);

FORT_API NTSTATUS ZwOpenSymbolicLinkObject(
        PHANDLE linkHandle, ACCESS_MASK desiredAccess, POBJECT_ATTRIBUTES objectAttributes);
FORT_API NTSTATUS ZwQuerySymbolicLinkObject(
        HANDLE linkHandle, PUNICODE_STRING linkTarget, PULONG returnedLength);

FORT_API NTSTATUS ZwQuerySystemInformation(ULONG systemInformationClass, PVOID systemInformation,
        ULONG systemInformationLength, PULONG returnLength);

FORT_API NTSTATUS ZwQueryInformationProcess(HANDLE processHandle, ULONG processInformationClass,
        PVOID processInformation, ULONG processInformationLength, PULONG returnLength);

FORT_API NTSTATUS MmCopyVirtualMemory(PEPROCESS sourceProcess, PVOID sourceAddress,
        PEPROCESS targetProcess, PVOID targetAddress, SIZE_T bufferSize,
        KPROCESSOR_MODE previousMode, PSIZE_T returnSize);

extern POBJECT_TYPE *PsProcessType;

FORT_API NTSTATUS ObReferenceObjectByHandle(HANDLE handle, ACCESS_MASK desiredAccess,
        POBJECT_TYPE objectType, KPROCESSOR_MODE accessMode, PVOID *object,
        POBJECT_HANDLE_INFORMATION handleInformation);

FORT_API ULONG DbgPrint(PCSTR format, ...);

#define ERROR_LOG_MAXIMUM_SIZE 240

typedef struct _IO_ERROR_LOG_PACKET
{
    UCHAR MajorFunctionCode;
    UCHAR RetryCount;
    USHORT DumpDataSize;
    USHORT NumberOfStrings;
    USHORT StringOffset;
    USHORT EventCategory;
    NTSTATUS ErrorCode;
    ULONG UniqueErrorValue;
    NTSTATUS FinalStatus;
    ULONG SequenceNumber;
    ULONG IoControlCode;
    LARGE_INTEGER DeviceOffset;
    ULONG DumpData[1];
} IO_ERROR_LOG_PACKET, *PIO_ERROR_LOG_PACKET;

FORT_API PVOID IoAllocateErrorLogEntry(PVOID ioObject, UCHAR entrySize);
FORT_API void IoWriteErrorLogEntry(PVOID elEntry);

FORT_API NTSTATUS IoRegisterShutdownNotification(PDEVICE_OBJECT deviceObject);
FORT_API void IoUnregisterShutdownNotification(PDEVICE_OBJECT deviceObject);

FORT_API PVOID IoGetInitialStack(void);
FORT_API void IoGetStackLimits(PULONG_PTR lowLimit, PULONG_PTR highLimit);
FORT_API ULONG_PTR IoGetRemainingStackSize(void);

FORT_API void KeBugCheckEx(ULONG bugCheckCode, ULONG_PTR bugCheckParameter1,
        ULONG_PTR bugCheckParameter2, ULONG_PTR bugCheckParameter3, ULONG_PTR bugCheckParameter4);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_WDM_H
