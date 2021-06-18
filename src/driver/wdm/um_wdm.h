#ifndef UM_WDM_H
#define UM_WDM_H

#include "../common/common.h"

#pragma warning(push)
#pragma warning(disable : 4005) // suppress warning: C4005: macro redefinition
#include <winternl.h>
#include <ntstatus.h>
#include <ifdef.h>
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

typedef struct
{
    union {
        struct
        {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            ULONG IoControlCode;
        } DeviceIoControl;
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

typedef LONG KTIMER, *PKTIMER;

typedef LONG *PIO_WORKITEM;
typedef VOID IO_WORKITEM_ROUTINE_EX(PVOID IoObject, PVOID Context, PIO_WORKITEM IoWorkItem);
typedef IO_WORKITEM_ROUTINE_EX *PIO_WORKITEM_ROUTINE_EX;

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

#define DPFLTR_IHVNETWORK_ID 0
#define DPFLTR_ERROR_LEVEL   0
FORT_API ULONG DbgPrintEx(ULONG componentId, ULONG level, PCSTR format, ...);

#define NT_ASSERT(cond) assert((cond))

#define NonPagedPool 0
FORT_API PVOID ExAllocatePoolWithTag(PVOID type, SIZE_T size, ULONG tag);
FORT_API void ExFreePoolWithTag(PVOID p, ULONG tag);

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

#define IO_NO_INCREMENT 0
FORT_API void IoCompleteRequest(PIRP irp, CCHAR priorityBoost);

FORT_API void KeInitializeTimer(PKTIMER timer);
FORT_API BOOLEAN KeCancelTimer(PKTIMER timer);
FORT_API BOOLEAN KeSetCoalescableTimer(
        PKTIMER timer, LARGE_INTEGER dueTime, ULONG period, ULONG tolerableDelay, PKDPC dpc);

FORT_API PIO_WORKITEM IoAllocateWorkItem(PDEVICE_OBJECT device);
FORT_API void IoFreeWorkItem(PIO_WORKITEM workItem);

#define DelayedWorkQueue 0
FORT_API void IoQueueWorkItemEx(
        PIO_WORKITEM workItem, PIO_WORKITEM_ROUTINE_EX routine, int queueType, PVOID context);

FORT_API void KeQuerySystemTime(PLARGE_INTEGER time);
FORT_API void ExSystemTimeToLocalTime(PLARGE_INTEGER systemTime, PLARGE_INTEGER localTime);
FORT_API void RtlTimeToTimeFields(PLARGE_INTEGER time, PTIME_FIELDS timeFields);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_WDM_H
