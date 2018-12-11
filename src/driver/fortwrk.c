/* Fort Firewall Worker for PASSIVE_LEVEL calls */

#define FORT_WORKER_REAUTH	0x01

typedef struct fort_worker {
  LONG volatile id_bits;

  PIO_WORKITEM item;
} FORT_WORKER, *PFORT_WORKER;


static void
fort_worker_reauth (void)
{
  NTSTATUS status;

  /* Force reauth filter */
  status = fort_prov_reauth(NULL);

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Worker Reauth: Error: %x\n", status);
  }
}

static void
fort_worker_callback (PVOID device, PVOID context, PIO_WORKITEM item)
{
  PFORT_WORKER worker = (PFORT_WORKER) context;
  const LONG id_bits = InterlockedAnd(&worker->id_bits, 0);

  UNUSED(device);
  UNUSED(item);

  if (id_bits & FORT_WORKER_REAUTH) {
    fort_worker_reauth();
  }
}

static void
fort_worker_queue (PFORT_WORKER worker, int work_id)
{
  const LONG id_bits = InterlockedOr(&worker->id_bits, work_id);

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
