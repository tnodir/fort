/* Fort Firewall Worker for PASSIVE_LEVEL calls */

#define FORT_WORKER_REAUTH	0x01

typedef void (*FORT_WORKER_FUNC) (void);

typedef struct fort_worker {
  UCHAR volatile id_bits;

  FORT_WORKER_FUNC reauth_func;

  PIO_WORKITEM item;
} FORT_WORKER, *PFORT_WORKER;


static void
fort_worker_callback (PVOID device, PVOID context, PIO_WORKITEM item)
{
  PFORT_WORKER worker = (PFORT_WORKER) context;
  const UCHAR id_bits = InterlockedAnd8(&worker->id_bits, 0);

  UNUSED(device);
  UNUSED(item);

  if (id_bits & FORT_WORKER_REAUTH) {
    worker->reauth_func();
  }
}

static void
fort_worker_queue (PFORT_WORKER worker, UCHAR work_id,
                   FORT_WORKER_FUNC worker_func)
{
  const UCHAR id_bits = InterlockedOr8(&worker->id_bits, work_id);

  if (work_id == FORT_WORKER_REAUTH) {
    worker->reauth_func = worker_func;
  }

  if (id_bits == 0) {
    IoQueueWorkItemEx(worker->item, &fort_worker_callback,
      DelayedWorkQueue, worker);
  }
}

static NTSTATUS
fort_worker_register (PDEVICE_OBJECT device, PFORT_WORKER worker)
{
  PIO_WORKITEM item = IoAllocateWorkItem(device);
  if (item == NULL) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  worker->item = item;

  return STATUS_SUCCESS;
}

static void
fort_worker_unregister (PFORT_WORKER worker)
{
  if (worker->item != NULL) {
    IoFreeWorkItem(worker->item);
  }
}
