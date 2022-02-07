/* Fort Firewall Worker for PASSIVE_LEVEL calls */

#include "fortwrk.h"

#include "fortcb.h"

static void NTAPI fort_worker_callback(PVOID device, PVOID context, PIO_WORKITEM item)
{
    PFORT_WORKER worker = (PFORT_WORKER) context;
    const UCHAR id_bits = InterlockedAnd8(&worker->id_bits, 0);

    UNUSED(device);
    UNUSED(item);

    if (id_bits & FORT_WORKER_REAUTH) {
        worker->reauth_func();
    }

    if (id_bits & FORT_WORKER_PSTREE) {
        worker->pstree_func();
    }
}

FORT_API void fort_worker_queue(PFORT_WORKER worker, UCHAR work_id, FORT_WORKER_FUNC worker_func)
{
    const UCHAR id_bits = InterlockedOr8(&worker->id_bits, work_id);

    switch (work_id) {
    case FORT_WORKER_REAUTH: {
        worker->reauth_func = worker_func;
    } break;
    case FORT_WORKER_PSTREE: {
        worker->pstree_func = worker_func;
    } break;
    }

    if (id_bits == 0) {
        IoQueueWorkItemEx(worker->item,
                FORT_CALLBACK(FORT_WORKER_CALLBACK, PIO_WORKITEM_ROUTINE_EX, fort_worker_callback),
                DelayedWorkQueue, worker);
    }
}

FORT_API NTSTATUS fort_worker_register(PDEVICE_OBJECT device, PFORT_WORKER worker)
{
    PIO_WORKITEM item = IoAllocateWorkItem(device);
    if (item == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    worker->item = item;

    return STATUS_SUCCESS;
}

FORT_API void fort_worker_unregister(PFORT_WORKER worker)
{
    if (worker->item != NULL) {
        IoFreeWorkItem(worker->item);
    }
}
