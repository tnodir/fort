/* Windows IP Filter Log Buffer */

typedef struct wipf_buffer {
  UINT32 top, size;
  PVOID data;
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
    ExFreePoolWithTag(buf->data, WIPF_POOL_TAG);
  }
}

static BOOL
wipf_buffer_write (PWIPF_BUFFER buf, UINT32 remote_ip, UINT64 pid,
                   UINT32 path_len, const PVOID path)
{
  return TRUE;
}
