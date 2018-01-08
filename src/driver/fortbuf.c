/* Fort Firewall Log Buffer */

#define FORT_BUFFER_POOL_TAG	'BwfF'

typedef struct fort_buffer_data {
  struct fort_buffer_data *next;

  UINT32 top;
  CHAR p[FORT_BUFFER_SIZE];
} FORT_BUFFER_DATA, *PFORT_BUFFER_DATA;

typedef struct fort_buffer {
  PFORT_BUFFER_DATA data_head;
  PFORT_BUFFER_DATA data_tail;  /* last is current */
  PFORT_BUFFER_DATA data_free;

  PIRP irp;  /* pending */
  PCHAR out;
  ULONG out_len;
  UINT32 out_top;

  KSPIN_LOCK lock;
} FORT_BUFFER, *PFORT_BUFFER;


static PFORT_BUFFER_DATA
fort_buffer_data_new (PFORT_BUFFER buf)
{
  PFORT_BUFFER_DATA data = buf->data_free;

  if (data != NULL) {
    buf->data_free = data->next;
  } else {
    data = fort_mem_alloc(sizeof(FORT_BUFFER_DATA), FORT_BUFFER_POOL_TAG);
  }

  return data;
}

static void
fort_buffer_data_del (PFORT_BUFFER_DATA data)
{
  while (data != NULL) {
    PFORT_BUFFER_DATA next = data->next;
    fort_mem_free(data, FORT_BUFFER_POOL_TAG);
    data = next;
  }
}

static PFORT_BUFFER_DATA
fort_buffer_data_alloc (PFORT_BUFFER buf, UINT32 len)
{
  PFORT_BUFFER_DATA data = buf->data_tail;

  if (data == NULL || len > FORT_BUFFER_SIZE - data->top) {
    PFORT_BUFFER_DATA new_data = fort_buffer_data_new(buf);

    if (new_data == NULL)
      return NULL;

    new_data->top = 0;
    new_data->next = NULL;

    if (data == NULL) {
      buf->data_head = new_data;
    } else {
      data->next = new_data;
    }

    buf->data_tail = new_data;

    data = new_data;
  }

  return data;
}

static void
fort_buffer_data_free (PFORT_BUFFER buf)
{
  PFORT_BUFFER_DATA data = buf->data_head;

  buf->data_head = data->next;

  if (data->next == NULL) {
    buf->data_tail = NULL;
  }

  data->next = buf->data_free;
  buf->data_free = data;
}

static void
fort_buffer_open (PFORT_BUFFER buf)
{
  KeInitializeSpinLock(&buf->lock);
}

static void
fort_buffer_close (PFORT_BUFFER buf)
{
  fort_buffer_data_del(buf->data_head);
  fort_buffer_data_del(buf->data_free);
}

static NTSTATUS
fort_buffer_prepare (PFORT_BUFFER buf, UINT32 len, PCHAR *out,
                     PIRP *irp, ULONG_PTR *info)
{
  /* Check pending buffer */
  if (buf->out_len && buf->out_top < buf->out_len) {
    const UINT32 out_top = buf->out_top;
    UINT32 new_top = out_top + len;

    /* Is it time to flush logs? */
    if (buf->out_len - new_top < FORT_LOG_SIZE_MAX) {
      if (irp != NULL) {
        buf->out_len = 0;

        *irp = buf->irp;
        buf->irp = NULL;

        *info = new_top;
        new_top = 0;
      } else {
        buf->out_len = out_top;
        new_top = out_top;
      }
    }

    *out = buf->out + out_top;
    buf->out_top = new_top;
  } else {
    PFORT_BUFFER_DATA data = fort_buffer_data_alloc(buf, len);
    const UINT32 buf_top = data ? data->top : FORT_BUFFER_SIZE;
    const UINT32 new_top = buf_top + len;

    if (new_top > FORT_BUFFER_SIZE) {
      return STATUS_BUFFER_TOO_SMALL;  /* drop on buffer overflow */
    }

    *out = data->p + buf_top;
    data->top = new_top;
  }

  return STATUS_SUCCESS;
}

static NTSTATUS
fort_buffer_blocked_write (PFORT_BUFFER buf, UINT32 remote_ip, UINT32 pid,
                           UINT32 path_len, const PVOID path,
                           PIRP *irp, ULONG_PTR *info)
{
  PCHAR out;
  UINT32 len;
  KLOCK_QUEUE_HANDLE lock_queue;
  NTSTATUS status;

  if (path_len > FORT_LOG_PATH_MAX) {
    path_len = 0;  /* drop too long path */
  }

  len = FORT_LOG_BLOCKED_SIZE(path_len);

  KeAcquireInStackQueuedSpinLock(&buf->lock, &lock_queue);

  status = fort_buffer_prepare(buf, len, &out, irp, info);

  if (NT_SUCCESS(status)) {
    fort_log_blocked_write(out, remote_ip, pid, path_len, path);
  }

  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static NTSTATUS
fort_buffer_proc_new_write (PFORT_BUFFER buf, UINT32 pid,
                            UINT32 path_len, const PVOID path,
                            PIRP *irp, ULONG_PTR *info)
{
  PCHAR out;
  UINT32 len;
  KLOCK_QUEUE_HANDLE lock_queue;
  NTSTATUS status;

  if (path_len > FORT_LOG_PATH_MAX) {
    path_len = 0;  /* drop too long path */
  }

  len = FORT_LOG_PROC_NEW_SIZE(path_len);

  KeAcquireInStackQueuedSpinLock(&buf->lock, &lock_queue);

  status = fort_buffer_prepare(buf, len, &out, irp, info);

  if (NT_SUCCESS(status)) {
    fort_log_proc_new_write(out, pid, path_len, path);
  }

  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static NTSTATUS
fort_buffer_xmove (PFORT_BUFFER buf, PIRP irp, PVOID out, ULONG out_len,
                   ULONG_PTR *info)
{
  PFORT_BUFFER_DATA data;
  UINT32 buf_top;
  KLOCK_QUEUE_HANDLE lock_queue;
  NTSTATUS status = STATUS_SUCCESS;

  KeAcquireInStackQueuedSpinLock(&buf->lock, &lock_queue);

  data = buf->data_head;
  *info = buf_top = (data ? data->top : 0);

  if (!buf_top) {
    if (buf->out_len) {
      status = STATUS_UNSUCCESSFUL;
    } else if (out_len < FORT_LOG_SIZE_MAX) {
      status = STATUS_BUFFER_TOO_SMALL;
    } else {
      buf->irp = irp;
      buf->out = out;
      buf->out_len = out_len;
      status = STATUS_PENDING;
    }
    goto end;
  }

  if (out_len < buf_top) {
    status = STATUS_BUFFER_TOO_SMALL;
    goto end;
  }

  RtlCopyMemory(out, data->p, buf_top);

  fort_buffer_data_free(buf);

 end:
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static NTSTATUS
fort_buffer_cancel_pending (PFORT_BUFFER buf, PIRP irp, ULONG_PTR *info)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  NTSTATUS status = STATUS_CANCELLED;

  *info = 0;

  KeAcquireInStackQueuedSpinLock(&buf->lock, &lock_queue);
  if (irp == buf->irp) {
    buf->irp = NULL;
    buf->out_len = 0;

    if (buf->out_top) {
      *info = buf->out_top;
      buf->out_top = 0;

      status = STATUS_SUCCESS;
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static void
fort_buffer_dpc_begin (PFORT_BUFFER buf, PKLOCK_QUEUE_HANDLE lock_queue)
{
  KeAcquireInStackQueuedSpinLockAtDpcLevel(&buf->lock, lock_queue);
}

static void
fort_buffer_dpc_end (PKLOCK_QUEUE_HANDLE lock_queue)
{
  KeReleaseInStackQueuedSpinLockFromDpcLevel(lock_queue);
}

static void
fort_buffer_dpc_flush_pending (PFORT_BUFFER buf, PIRP *irp, ULONG_PTR *info)
{
  const UINT32 out_top = buf->out_top;

  if (out_top) {
    *info = out_top;

    buf->out_top = 0;
    buf->out_len = 0;

    *irp = buf->irp;
    buf->irp = NULL;
  }
}
