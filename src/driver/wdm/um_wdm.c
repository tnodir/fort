#include "um_wdm.h"

ULONG DbgPrintEx(ULONG componentId, ULONG level, PCSTR format, ...)
{
    UNUSED(componentId);
    UNUSED(level);
    UNUSED(format);
    return 0;
}

PVOID ExAllocatePoolWithTag(PVOID type, SIZE_T size, ULONG tag)
{
    UNUSED(type);
    UNUSED(tag);
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void ExFreePoolWithTag(PVOID p, ULONG tag)
{
    UNUSED(tag);
    HeapFree(GetProcessHeap(), 0, p);
}

void ExFreePool(PVOID p)
{
    UNUSED(p);
}

PVOID ExAllocatePool2(POOL_FLAGS flags, SIZE_T size, ULONG tag)
{
    UNUSED(flags);
    UNUSED(tag);
    return HeapAlloc(GetProcessHeap(), 0, size);
}

PIO_STACK_LOCATION IoGetCurrentIrpStackLocation(PIRP irp)
{
    UNUSED(irp);
    return NULL;
}

void IoMarkIrpPending(PIRP irp)
{
    UNUSED(irp);
}

PDRIVER_CANCEL IoSetCancelRoutine(PIRP irp, PDRIVER_CANCEL routine)
{
    UNUSED(irp);
    UNUSED(routine);
    return NULL;
}

void KeInitializeDpc(PRKDPC dpc, PKDEFERRED_ROUTINE routine, PVOID context)
{
    UNUSED(dpc);
    UNUSED(routine);
    UNUSED(context);
}

void KeFlushQueuedDpcs(void) { }

void ExInitializeDriverRuntime(ULONG runtimeFlags)
{
    UNUSED(runtimeFlags);
}

NTSTATUS IoCreateDevice(PDRIVER_OBJECT driver, ULONG extensionSize, PUNICODE_STRING name,
        DWORD type, ULONG characteristics, BOOLEAN exclusive, PDEVICE_OBJECT *device)
{
    UNUSED(driver);
    UNUSED(extensionSize);
    UNUSED(name);
    UNUSED(type);
    UNUSED(characteristics);
    UNUSED(exclusive);
    UNUSED(device);
    return STATUS_SUCCESS;
}

void IoDeleteDevice(PDEVICE_OBJECT device)
{
    UNUSED(device);
}

NTSTATUS IoCreateSymbolicLink(PUNICODE_STRING symbolicLink, PUNICODE_STRING deviceName)
{
    UNUSED(symbolicLink);
    UNUSED(deviceName);
    return STATUS_SUCCESS;
}

NTSTATUS IoDeleteSymbolicLink(PUNICODE_STRING symbolicLink)
{
    UNUSED(symbolicLink);
    return STATUS_SUCCESS;
}

NTSTATUS KeDelayExecutionThread(
        KPROCESSOR_MODE waitMode, BOOLEAN alertable, PLARGE_INTEGER interval)
{
    UNUSED(waitMode);
    UNUSED(alertable);
    UNUSED(interval);
    return STATUS_SUCCESS;
}

NTSTATUS ExCreateCallback(
        PCALLBACK_OBJECT *callback, POBJECT_ATTRIBUTES attr, BOOLEAN create, BOOLEAN multiple)
{
    UNUSED(callback);
    UNUSED(attr);
    UNUSED(create);
    UNUSED(multiple);
    return STATUS_SUCCESS;
}

PVOID ExRegisterCallback(PCALLBACK_OBJECT callback, PCALLBACK_FUNCTION function, PVOID context)
{
    UNUSED(callback);
    UNUSED(function);
    UNUSED(context);
    return NULL;
}

void ExUnregisterCallback(PVOID callbackReg)
{
    UNUSED(callbackReg);
}

LONG_PTR ObDereferenceObject(PVOID object)
{
    UNUSED(object);
    return 0;
}

void KeInitializeSpinLock(PKSPIN_LOCK lock)
{
    UNUSED(lock);
}

void KeAcquireInStackQueuedSpinLock(PKSPIN_LOCK lock, PKLOCK_QUEUE_HANDLE handle)
{
    UNUSED(lock);
    UNUSED(handle);
}

void KeReleaseInStackQueuedSpinLock(PKLOCK_QUEUE_HANDLE handle)
{
    UNUSED(handle);
}

void KeAcquireInStackQueuedSpinLockAtDpcLevel(PKSPIN_LOCK lock, PKLOCK_QUEUE_HANDLE handle)
{
    UNUSED(lock);
    UNUSED(handle);
}

void KeReleaseInStackQueuedSpinLockFromDpcLevel(PKLOCK_QUEUE_HANDLE handle)
{
    UNUSED(handle);
}

void IoAcquireCancelSpinLock(PKIRQL irql)
{
    UNUSED(irql);
}

void IoReleaseCancelSpinLock(KIRQL irql)
{
    UNUSED(irql);
}

KIRQL ExAcquireSpinLockShared(PEX_SPIN_LOCK lock)
{
    UNUSED(lock);
    return 0;
}

KIRQL ExAcquireSpinLockExclusive(PEX_SPIN_LOCK lock)
{
    UNUSED(lock);
    return 0;
}

void ExReleaseSpinLockShared(PEX_SPIN_LOCK lock, KIRQL oldIrql)
{
    UNUSED(lock);
    UNUSED(oldIrql);
}

void ExReleaseSpinLockExclusive(PEX_SPIN_LOCK lock, KIRQL oldIrql)
{
    UNUSED(lock);
    UNUSED(oldIrql);
}

void IoCompleteRequest(PIRP irp, CCHAR priorityBoost)
{
    UNUSED(irp);
    UNUSED(priorityBoost);
}

void KeInitializeTimer(PKTIMER timer)
{
    UNUSED(timer);
}

BOOLEAN KeCancelTimer(PKTIMER timer)
{
    UNUSED(timer);
    return FALSE;
}

BOOLEAN KeSetCoalescableTimer(
        PKTIMER timer, LARGE_INTEGER dueTime, ULONG period, ULONG tolerableDelay, PKDPC dpc)
{
    UNUSED(timer);
    UNUSED(dueTime);
    UNUSED(period);
    UNUSED(tolerableDelay);
    UNUSED(dpc);
    return FALSE;
}

PIO_WORKITEM IoAllocateWorkItem(PDEVICE_OBJECT device)
{
    UNUSED(device);
    return NULL;
}

void IoFreeWorkItem(PIO_WORKITEM workItem)
{
    UNUSED(workItem);
}

void KeQuerySystemTime(PLARGE_INTEGER time)
{
    UNUSED(time);
}

void ExSystemTimeToLocalTime(PLARGE_INTEGER systemTime, PLARGE_INTEGER localTime)
{
    UNUSED(systemTime);
    UNUSED(localTime);
}

void RtlTimeToTimeFields(PLARGE_INTEGER time, PTIME_FIELDS timeFields)
{
    UNUSED(time);
    UNUSED(timeFields);
}

void IoQueueWorkItemEx(
        PIO_WORKITEM workItem, PIO_WORKITEM_ROUTINE_EX routine, int queueType, PVOID context)
{
    UNUSED(workItem);
    UNUSED(routine);
    UNUSED(queueType);
    UNUSED(context);
}
