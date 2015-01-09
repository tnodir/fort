/* Windows IP Filter Log */

static void
wipf_log_write (char *p, UINT32 remote_ip, UINT64 pid,
                UINT32 path_len, const char *path)
{
  *((UINT32 *) p)++ = remote_ip;
  *((UINT64 *) p)++ = pid;
  *((UINT32 *) p)++ = path_len;

  if (path_len) {
    RtlCopyMemory(p, path, path_len);
  }
}

static void
wipf_log_read (const char **p, UINT32 *remote_ip, UINT64 *pid,
               UINT32 *path_len, const char **path)
{
  *remote_ip = *((UINT32 *) *p)++;
  *pid = *((UINT64 *) *p)++;
  *path_len = *((UINT32 *) *p)++;

  if (*path_len) {
    *path = *p;
    *p += *path_len;
  }
}

