/* Fort Firewall Thread */

#include "fortthr.h"

#include "fortcb.h"
#include "fortdbg.h"

FORT_API NTSTATUS fort_thread_run(
        PFORT_THREAD thread, PKSTART_ROUTINE routine, PVOID context, int priorityIncrement)
{
    NTSTATUS status;

    thread->thread_obj = NULL;

    HANDLE hThread;
    status = PsCreateSystemThread(&hThread, 0, NULL, NULL, NULL, routine, context);
    if (!NT_SUCCESS(status))
        return status;

    status = ObReferenceObjectByHandle(
            hThread, THREAD_ALL_ACCESS, NULL, KernelMode, &thread->thread_obj, NULL);

    ZwClose(hThread);

    if (NT_SUCCESS(status) && priorityIncrement != 0) {
        KeSetBasePriorityThread(thread->thread_obj, priorityIncrement);
    }

    return status;
}

FORT_API void fort_thread_wait(PFORT_THREAD thread)
{
    if (thread->thread_obj == NULL)
        return;

    KeWaitForSingleObject(thread->thread_obj, Executive, KernelMode, FALSE, NULL);

    ObDereferenceObject(thread->thread_obj);
    thread->thread_obj = NULL;
}
