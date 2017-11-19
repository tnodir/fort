/* Fort Firewall Log Buffer */

#define FORT_BUFFER_POOL_TAG	'BwfF'

#include "../common/fortlog.c"

typedef struct fort_buffer {
  UINT32 top;
  PCHAR data;

  PIRP irp;  /* pending */
  PCHAR out;
  ULONG out_len;

  KSPIN_LOCK lock;
} FORT_BUFFER, *PFORT_BUFFER;


static void
fort_buffer_init (PFORT_BUFFER buf)
{
  buf->data = ExAllocatePoolWithTag(NonPagedPool, FORT_BUFFER_SIZE,
                                    FORT_BUFFER_POOL_TAG);

  KeInitializeSpinLock(&buf->lock);
}

static void
fort_buffer_close (PFORT_BUFFER buf)
{
  if (buf->data != NULL) {
    ExFreePoolWithTag(buf->data, FORT_BUFFER_POOL_TAG);

    buf->data = NULL;
    buf->top = 0;
  }
}

static NTSTATUS
fort_buffer_blocked_write (PFORT_BUFFER buf, UINT32 remote_ip, UINT32 pid,
                           UINT32 path_len, const PVOID path,
                           PIRP *irp, NTSTATUS *irp_status, ULONG_PTR *info)
{
  UINT32 len, new_top;
  PCHAR out;
  KIRQL irq;
  NTSTATUS status = STATUS_SUCCESS;

  if (path_len > FORT_LOG_PATH_MAX) {
    path_len = 0;  /* drop too long path */
  }

  len = FORT_LOG_BLOCKED_SIZE(path_len);

  KeAcquireSpinLock(&buf->lock, &irq);

  new_top = buf->top + len;

  /* Try to directly write to pending client */
  if (buf->irp != NULL) {
    if (FORT_LOG_BLOCKED_SIZE_MAX > buf->out_len - new_top) {
      *irp = buf->irp;
      buf->irp = NULL;

      *irp_status = STATUS_SUCCESS;

      *info = new_top;
      new_top = 0;
    }

    out = buf->out;
  } else {
    if (new_top > FORT_BUFFER_SIZE) {
      status = STATUS_BUFFER_TOO_SMALL;
      goto end;  /* drop on buffer overflow */
    }

    out = buf->data;
  }

  out += buf->top;

  fort_log_blocked_write(out, remote_ip, pid, path_len, path);

  buf->top = new_top;

 end:
  KeReleaseSpinLock(&buf->lock, irq);

  return status;
}

static NTSTATUS
fort_buffer_xmove (PFORT_BUFFER buf, PIRP irp, PVOID out, ULONG out_len,
                   ULONG_PTR *info)
{
  KIRQL irq;
  NTSTATUS status = STATUS_SUCCESS;

  KeAcquireSpinLock(&buf->lock, &irq);

  *info = buf->top;

  if (!buf->top) {
    if (buf->irp != NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
    } else if (out_len < FORT_LOG_BLOCKED_SIZE_MAX) {
      status = STATUS_BUFFER_TOO_SMALL;
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

static NTSTATUS
fort_buffer_cancel_pending (PFORT_BUFFER buf, PIRP irp, ULONG_PTR *info)
{
  NTSTATUS status = STATUS_CANCELLED;
  KIRQL irq;

  *info = 0;

  KeAcquireSpinLock(&buf->lock, &irq);
  if (irp == buf->irp) {
    buf->irp = NULL;

    if (buf->top) {
      *info = buf->top;
      buf->top = 0;

      status = STATUS_SUCCESS;
    }
  }
  KeReleaseSpinLock(&buf->lock, irq);

  return status;
}
