#include "um_wdm.h"

ULONG DbgPrintEx(ULONG componentId, ULONG level, PCSTR format, ...)
{
    UNUSED(componentId);
    UNUSED(level);
    UNUSED(format);
    return 0;
}

PEPROCESS IoGetCurrentProcess(void)
{
    return NULL;
}

PVOID ExAllocatePoolWithTag(DWORD type, SIZE_T size, ULONG tag)
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

PVOID ExAllocatePool(DWORD flags, SIZE_T size)
{
    UNUSED(flags);
    return HeapAlloc(GetProcessHeap(), 0, size);
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

KIRQL KeGetCurrentIrql(void)
{
    return 0;
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

void ExInitializeRundownProtection(PEX_RUNDOWN_REF runRef)
{
    UNUSED(runRef);
}

BOOLEAN ExAcquireRundownProtection(PEX_RUNDOWN_REF runRef)
{
    UNUSED(runRef);
    return FALSE;
}

void ExReleaseRundownProtection(PEX_RUNDOWN_REF runRef)
{
    UNUSED(runRef);
}

void KeInitializeEvent(PRKEVENT event, EVENT_TYPE type, BOOLEAN state)
{
    UNUSED(event);
    UNUSED(type);
    UNUSED(state);
}

void KeClearEvent(PRKEVENT event)
{
    UNUSED(event);
}

LONG KeSetEvent(PRKEVENT event, KPRIORITY increment, BOOLEAN wait)
{
    UNUSED(event);
    UNUSED(increment);
    UNUSED(wait);
    return 0;
}

NTSTATUS KeWaitForSingleObject(PVOID object, KWAIT_REASON waitReason, KPROCESSOR_MODE waitMode,
        BOOLEAN alertable, PLARGE_INTEGER timeout)
{
    UNUSED(object);
    UNUSED(waitReason);
    UNUSED(waitMode);
    UNUSED(alertable);
    UNUSED(timeout);
    return STATUS_SUCCESS;
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

void IoQueueWorkItem(
        PIO_WORKITEM workItem, PIO_WORKITEM_ROUTINE routine, int queueType, PVOID context)
{
    UNUSED(workItem);
    UNUSED(routine);
    UNUSED(queueType);
    UNUSED(context);
}

void IoQueueWorkItemEx(
        PIO_WORKITEM workItem, PIO_WORKITEM_ROUTINE_EX routine, int queueType, PVOID context)
{
    UNUSED(workItem);
    UNUSED(routine);
    UNUSED(queueType);
    UNUSED(context);
}

NTSTATUS PsCreateSystemThread(PHANDLE threadHandle, ULONG desiredAccess,
        POBJECT_ATTRIBUTES objectAttributes, HANDLE processHandle, PVOID clientId,
        PKSTART_ROUTINE startRoutine, PVOID startContext)
{
    UNUSED(threadHandle);
    UNUSED(desiredAccess);
    UNUSED(objectAttributes);
    UNUSED(processHandle);
    UNUSED(clientId);
    UNUSED(startRoutine);
    UNUSED(startContext);
    return STATUS_SUCCESS;
}

LARGE_INTEGER KeQueryPerformanceCounter(PLARGE_INTEGER performanceFrequency)
{
    UNUSED(performanceFrequency);
    const LARGE_INTEGER res = { 0 };
    return res;
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

NTSTATUS RtlGetVersion(PRTL_OSVERSIONINFOW versionInformation)
{
    UNUSED(versionInformation);
    return STATUS_SUCCESS;
}

NTSTATUS ZwOpenKey(
        PHANDLE keyHandle, ACCESS_MASK desiredAccess, POBJECT_ATTRIBUTES objectAttributes)
{
    UNUSED(keyHandle);
    UNUSED(desiredAccess);
    UNUSED(objectAttributes);
    return STATUS_SUCCESS;
}

NTSTATUS ZwClose(HANDLE handle)
{
    UNUSED(handle);
    return STATUS_SUCCESS;
}

NTSTATUS ZwQueryValueKey(HANDLE keyHandle, PUNICODE_STRING valueName,
        KEY_VALUE_INFORMATION_CLASS keyValueInformationClass, PVOID keyValueInformation,
        ULONG length, PULONG resultLength)
{
    UNUSED(keyHandle);
    UNUSED(valueName);
    UNUSED(keyValueInformationClass);
    UNUSED(keyValueInformation);
    UNUSED(length);
    UNUSED(resultLength);
    return STATUS_SUCCESS;
}

NTSTATUS ZwOpenFile(PHANDLE fileHandle, ACCESS_MASK desiredAccess,
        POBJECT_ATTRIBUTES objectAttributes, PIO_STATUS_BLOCK ioStatusBlock, ULONG shareAccess,
        ULONG openOptions)
{
    UNUSED(fileHandle);
    UNUSED(desiredAccess);
    UNUSED(objectAttributes);
    UNUSED(ioStatusBlock);
    UNUSED(shareAccess);
    UNUSED(openOptions);
    return STATUS_SUCCESS;
}

NTSTATUS ZwQueryInformationFile(HANDLE fileHandle, PIO_STATUS_BLOCK ioStatusBlock,
        PVOID fileInformation, ULONG length, FILE_INFORMATION_CLASS fileInformationClass)
{
    UNUSED(fileHandle);
    UNUSED(ioStatusBlock);
    UNUSED(fileInformation);
    UNUSED(length);
    UNUSED(fileInformationClass);
    return STATUS_SUCCESS;
}

NTSTATUS ZwReadFile(HANDLE fileHandle, HANDLE event, PIO_APC_ROUTINE apcRoutine, PVOID apcContext,
        PIO_STATUS_BLOCK ioStatusBlock, PVOID buffer, ULONG length, PLARGE_INTEGER byteOffset,
        PULONG key)
{
    UNUSED(fileHandle);
    UNUSED(event);
    UNUSED(apcRoutine);
    UNUSED(apcContext);
    UNUSED(ioStatusBlock);
    UNUSED(buffer);
    UNUSED(length);
    UNUSED(byteOffset);
    UNUSED(key);
    return STATUS_SUCCESS;
}

NTSTATUS ZwOpenDirectoryObject(
        PHANDLE directoryHandle, ACCESS_MASK desiredAccess, POBJECT_ATTRIBUTES objectAttributes)
{
    UNUSED(directoryHandle);
    UNUSED(desiredAccess);
    UNUSED(objectAttributes);
    return STATUS_SUCCESS;
}

NTSTATUS ZwOpenSymbolicLinkObject(
        PHANDLE linkHandle, ACCESS_MASK desiredAccess, POBJECT_ATTRIBUTES objectAttributes)
{
    UNUSED(linkHandle);
    UNUSED(desiredAccess);
    UNUSED(objectAttributes);
    return STATUS_SUCCESS;
}

NTSTATUS ZwQuerySymbolicLinkObject(
        HANDLE linkHandle, PUNICODE_STRING linkTarget, PULONG returnedLength)
{
    UNUSED(linkHandle);
    UNUSED(linkTarget);
    UNUSED(returnedLength);
    return STATUS_SUCCESS;
}

NTSTATUS ZwQuerySystemInformation(ULONG systemInformationClass, PVOID systemInformation,
        ULONG systemInformationLength, PULONG returnLength)
{
    UNUSED(systemInformationClass);
    UNUSED(systemInformation);
    UNUSED(systemInformationLength);
    UNUSED(returnLength);
    return STATUS_SUCCESS;
}

NTSTATUS ZwQueryInformationProcess(HANDLE processHandle, ULONG processInformationClass,
        PVOID processInformation, ULONG processInformationLength, PULONG returnLength)
{
    UNUSED(processHandle);
    UNUSED(processInformationClass);
    UNUSED(processInformation);
    UNUSED(processInformationLength);
    UNUSED(returnLength);
    return STATUS_SUCCESS;
}

NTSTATUS MmCopyVirtualMemory(PEPROCESS sourceProcess, PVOID sourceAddress, PEPROCESS targetProcess,
        PVOID targetAddress, SIZE_T bufferSize, KPROCESSOR_MODE previousMode, PSIZE_T returnSize)
{
    UNUSED(sourceProcess);
    UNUSED(sourceAddress);
    UNUSED(targetProcess);
    UNUSED(targetAddress);
    UNUSED(bufferSize);
    UNUSED(previousMode);
    UNUSED(returnSize);
    return STATUS_SUCCESS;
}

POBJECT_TYPE *PsProcessType = NULL;

NTSTATUS ObReferenceObjectByHandle(HANDLE handle, ACCESS_MASK desiredAccess,
        POBJECT_TYPE objectType, KPROCESSOR_MODE accessMode, PVOID *object,
        POBJECT_HANDLE_INFORMATION handleInformation)
{
    UNUSED(handle);
    UNUSED(desiredAccess);
    UNUSED(objectType);
    UNUSED(accessMode);
    UNUSED(object);
    UNUSED(handleInformation);
    return STATUS_SUCCESS;
}

ULONG DbgPrint(PCSTR format, ...)
{
    UNUSED(format);
    return 0;
}

PVOID IoAllocateErrorLogEntry(PVOID ioObject, UCHAR entrySize)
{
    UNUSED(ioObject);
    UNUSED(entrySize);
    return NULL;
}

void IoWriteErrorLogEntry(PVOID elEntry)
{
    UNUSED(elEntry);
}

NTSTATUS IoRegisterShutdownNotification(PDEVICE_OBJECT deviceObject)
{
    UNUSED(deviceObject);
    return STATUS_SUCCESS;
}

void IoUnregisterShutdownNotification(PDEVICE_OBJECT deviceObject)
{
    UNUSED(deviceObject);
}

PVOID IoGetInitialStack(void)
{
    return NULL;
}

void IoGetStackLimits(PULONG_PTR lowLimit, PULONG_PTR highLimit)
{
    UNUSED(lowLimit);
    UNUSED(highLimit);
}

ULONG_PTR IoGetRemainingStackSize(void)
{
    return 0;
}

void KeBugCheckEx(ULONG bugCheckCode, ULONG_PTR bugCheckParameter1, ULONG_PTR bugCheckParameter2,
        ULONG_PTR bugCheckParameter3, ULONG_PTR bugCheckParameter4)
{
}
