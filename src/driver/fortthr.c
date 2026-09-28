/* Fort Firewall Thread */

#include "fortthr.h"

#include "fortcb.h"
#include "fortdbg.h"

static void fort_thread_set_priority(HANDLE hThread, int priorityIncrement)
{
    PVOID threadObj;
    const NTSTATUS status = ObReferenceObjectByHandle(
            hThread, THREAD_ALL_ACCESS, NULL, KernelMode, &threadObj, NULL);

    if (NT_SUCCESS(status)) {
        KeSetBasePriorityThread(threadObj, priorityIncrement);

        ObDereferenceObject(threadObj);
    }
}

FORT_API NTSTATUS fort_thread_run(
        PFORT_THREAD thread, PKSTART_ROUTINE routine, PVOID context, int priorityIncrement)
{
    NTSTATUS status;

    thread->thread_handle = NULL;

    OBJECT_ATTRIBUTES objectAttr;
    InitializeObjectAttributes(&objectAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE hThread;
    status = PsCreateSystemThread(
            &hThread, THREAD_ALL_ACCESS, &objectAttr, NULL, NULL, routine, context);
    if (!NT_SUCCESS(status))
        return status;

    /* Keep the handle to wait for the running thread */
    thread->thread_handle = hThread;

    if (priorityIncrement != 0) {
        fort_thread_set_priority(hThread, priorityIncrement);
    }

    return STATUS_SUCCESS;
}

FORT_API void fort_thread_wait(PFORT_THREAD thread)
{
    const HANDLE hThread = thread->thread_handle;
    if (hThread == NULL)
        return;

    ZwWaitForSingleObject(hThread, FALSE, NULL);

    ZwClose(hThread);
    thread->thread_handle = NULL;
}
