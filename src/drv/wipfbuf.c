/* Windows IP Filter Log Buffer */

#define WIPF_BUFFER_POOL_TAG	'IPFB'

#include "../wipflog.c"

typedef struct wipf_buffer {
  UINT32 top, size;
  PVOID data;

  PIRP irp;  /* pending */
  PVOID out;
  ULONG out_len;

  KSPIN_LOCK lock;
} WIPF_BUFFER, *PWIPF_BUFFER;


static void
wipf_buffer_init (PWIPF_BUFFER buf)
{
  KeInitializeSpinLock(&buf->lock);
}

static void
wipf_buffer_close (PWIPF_BUFFER buf)
{
  if (buf->data != NULL) {
    ExFreePoolWithTag(buf->data, WIPF_BUFFER_POOL_TAG);

    buf->data = NULL;
    buf->size = 0;
    buf->top = 0;
  }
}

static NTSTATUS
wipf_buffer_write (PWIPF_BUFFER buf, UINT32 remote_ip, UINT64 pid,
                   UINT32 path_len, const PVOID path,
                   PIRP *irp, NTSTATUS *irp_status, ULONG_PTR *info)
{
  UINT32 len = WIPF_LOG_SIZE(path_len);
  UINT32 size;
  KIRQL irq;
  NTSTATUS status = STATUS_SUCCESS;

  if (len > WIPF_BUFFER_SIZE) {
    path_len = 0;  /* drop too long path */
    len = WIPF_LOG_SIZE(0);
  }

  KeAcquireSpinLock(&buf->lock, &irq);

  /* Try to directly write to pending client */
  if (buf->irp) {
    *irp = buf->irp;
    buf->irp = NULL;

    *info = len;

    if (buf->out_len < len) {
      *irp_status = STATUS_BUFFER_TOO_SMALL;
      /* fallback to buffer */
    } else {
      wipf_log_write(buf->out, remote_ip, pid, path_len, path);

      *irp_status = STATUS_SUCCESS;
      goto end;
    }
  }

  size = buf->size;

  if (len > size - buf->top) {
    size *= 2;
  }
  if (size > WIPF_BUFFER_SIZE_MAX) {
    status = STATUS_BUFFER_TOO_SMALL;
    goto end;  /* drop on overflow of buffer */
  }

  /* resize the buffer */
  if (size != buf->size) {
    PVOID data = ExAllocatePoolWithTag(NonPagedPool, size, WIPF_BUFFER_POOL_TAG);
    if (data == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto end;  /* drop on OOM */
    }

    /* move old data to the new place */
    {
      const UINT32 top = buf->top;

      RtlCopyMemory(data, buf->data, top);
      wipf_buffer_close(buf);

      buf->data = data;
      buf->size = size;
      buf->top = top;
    }
  }

  buf->top += len;

  wipf_log_write(buf->data, remote_ip, pid, path_len, path);

 end:
  KeReleaseSpinLock(&buf->lock, irq);

  return status;
}

static NTSTATUS
wipf_buffer_xmove (PWIPF_BUFFER buf, PIRP irp, PVOID out, ULONG out_len,
                   ULONG_PTR *info)
{
  KIRQL irq;
  NTSTATUS status = STATUS_SUCCESS;

  KeAcquireSpinLock(&buf->lock, &irq);

  *info = buf->top;

  if (!buf->top) {
    if (buf->irp) {
      status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
      buf->irp = irp;
      buf->out = out;
      buf->out_len = out_len;
      status = STATUS_PENDING;
    }
    goto end;
  }

  if (out_len < buf->top) {
    status = STATUS_BUFFER_TOO_SMALL;
    goto end;
  }

  RtlCopyMemory(out, buf->data, buf->top);
  buf->top = 0;

 end:
  KeReleaseSpinLock(&buf->lock, irq);

  return status;
}

static void
wipf_buffer_cancel_pending (PWIPF_BUFFER buf, PIRP irp)
{
  KIRQL irq;

  KeAcquireSpinLock(&buf->lock, &irq);
  if (irp == buf->irp) {
    buf->irp = NULL;
  }
  KeReleaseSpinLock(&buf->lock, irq);
}
