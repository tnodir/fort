/* Fort Firewall Worker for PASSIVE_LEVEL calls */

#include "fortwrk.h"

#include <assert.h>

#include "fortdbg.h"
#include "fortutl.h"

static UCHAR fort_worker_flags_set(PFORT_WORKER worker, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&worker->flags, flags) : InterlockedAnd8(&worker->flags, ~flags);
}

static UCHAR fort_worker_flags(PFORT_WORKER worker)
{
    return fort_worker_flags_set(worker, 0, TRUE);
}

static void fort_worker_callback_run(
        PFORT_WORKER worker, enum FORT_WORKER_TYPE worker_type, UCHAR id_bits)
{
    if ((id_bits & (1 << worker_type)) != 0) {
        worker->funcs[worker_type]();
    }
}

static NTSTATUS fort_worker_callback_expand(PVOID context)
{
    PFORT_WORKER worker = context;

    const UCHAR id_bits = InterlockedAnd8(&worker->id_bits, 0);

    fort_worker_callback_run(worker, FORT_WORKER_REAUTH, id_bits);

    return STATUS_SUCCESS;
}

static void fort_worker_thread_loop(PVOID context)
{
    FORT_CHECK_STACK(FORT_WORKER_CALLBACK);

    PFORT_WORKER worker = context;

    for (;;) {
        KeWaitForSingleObject(&worker->thread_event, Executive, KernelMode, FALSE, NULL);

        if ((fort_worker_flags(worker) & FORT_WORKER_CLOSED) != 0)
            break;

        const NTSTATUS status = fort_expand_stack(&fort_worker_callback_expand, worker);
        UNUSED(status);
    }
}

FORT_API void fort_worker_func_set(PFORT_WORKER worker, UCHAR work_id, FORT_WORKER_FUNC worker_func)
{
    assert(work_id >= 0 && work_id < FORT_WORKER_FUNC_COUNT);

    worker->funcs[work_id] = worker_func;
}

FORT_API void fort_worker_queue(PFORT_WORKER worker, UCHAR work_id)
{
    const UCHAR id_bits = InterlockedOr8(&worker->id_bits, (1 << work_id));

    if (id_bits == 0) {
        KeSetEvent(&worker->thread_event, IO_NO_INCREMENT, FALSE);
    }
}

FORT_API NTSTATUS fort_worker_register(PFORT_WORKER worker)
{
    KeInitializeEvent(&worker->thread_event, SynchronizationEvent, FALSE);

    return fort_thread_run(
            &worker->thread, &fort_worker_thread_loop, worker, /*priorityIncrement=*/0);
}

FORT_API void fort_worker_unregister(PFORT_WORKER worker)
{
    if (worker->thread.thread_handle == NULL)
        return;

    /* The queued funcs aren't run after the close */
    fort_worker_flags_set(worker, FORT_WORKER_CLOSED, TRUE);

    KeSetEvent(&worker->thread_event, IO_NO_INCREMENT, FALSE);

    /* Wait for the thread's exit, then the driver's code isn't running by it */
    fort_thread_wait(&worker->thread);
}
