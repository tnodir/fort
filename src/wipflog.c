/* Windows IP Filter Log */

#define WIPF_BUFFER_SIZE	16 * 1024
#define WIPF_LOG_PATH_MAX	512
#define WIPF_LOG_ALIGN		4

#define WIPF_LOG_SIZE(path_len) \
  ((sizeof(UINT32) + sizeof(UINT32) + sizeof(UINT32) + (path_len)) \
     + (WIPF_LOG_ALIGN-1)) & ~(WIPF_LOG_ALIGN-1)


static void
wipf_log_write (char *p, UINT32 remote_ip, UINT32 pid,
                UINT32 path_len, const char *path)
{
  *((UINT32 *) p)++ = remote_ip;
  *((UINT32 *) p)++ = pid;
  *((UINT32 *) p)++ = path_len;

  if (path_len) {
    RtlCopyMemory(p, path, path_len);
  }
}

static void
wipf_log_read (const char *p, UINT32 *remote_ip, UINT32 *pid,
               UINT32 *path_len, const char **path)
{
  *remote_ip = *((UINT32 *) p)++;
  *pid = *((UINT32 *) p)++;
  *path_len = *((UINT32 *) p)++;
  *path = *path_len ? p : NULL;
}

