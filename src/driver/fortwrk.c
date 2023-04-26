/* Fort Firewall Worker for PASSIVE_LEVEL calls */

#include "fortwrk.h"

#include <assert.h>

#include "fortcb.h"
#include "forttrace.h"

static void fort_worker_callback_run(
        PFORT_WORKER worker, enum FORT_WORKER_TYPE worker_type, UCHAR id_bits)
{
    if ((id_bits & (1 << worker_type)) != 0) {
        worker->funcs[worker_type]();
    }
}

static void NTAPI fort_worker_callback(PDEVICE_OBJECT device, PVOID context)
{
    UNUSED(device);

    PFORT_WORKER worker = (PFORT_WORKER) context;

    InterlockedDecrement16(&worker->queue_size);

    const UCHAR id_bits = InterlockedAnd8(&worker->id_bits, 0);

    fort_worker_callback_run(worker, FORT_WORKER_REAUTH, id_bits);
    fort_worker_callback_run(worker, FORT_WORKER_PSTREE, id_bits);
}

static void fort_worker_wait(PFORT_WORKER worker)
{
    InterlockedAnd8(&worker->id_bits, 0);

    for (;;) {
        const SHORT queue_size = InterlockedOr16(&worker->queue_size, 0);

        LARGE_INTEGER timeout;
        timeout.QuadPart = -50 * 1000 * 10; /* 50 msecs */

        KeDelayExecutionThread(KernelMode, FALSE, &timeout);

        if (queue_size == 0)
            break; /* Check the extra one time to ensure thread's exit from callback function */
    }
}

FORT_API void fort_worker_func_set(PFORT_WORKER worker, UCHAR work_id, FORT_WORKER_FUNC worker_func)
{
    NT_ASSERT(work_id >= 0 && work_id < FORT_WORKER_FUNC_COUNT);

    worker->funcs[work_id] = worker_func;
}

FORT_API void fort_worker_queue(PFORT_WORKER worker, UCHAR work_id)
{
    const UCHAR id_bits = InterlockedOr8(&worker->id_bits, (1 << work_id));

    if (id_bits == 0) {
        InterlockedIncrement16(&worker->queue_size);

        IoQueueWorkItem(worker->item,
                FORT_CALLBACK(FORT_WORKER_CALLBACK, PIO_WORKITEM_ROUTINE, &fort_worker_callback),
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
        fort_worker_wait(worker);

        IoFreeWorkItem(worker->item);
        worker->item = NULL;
    }
}
