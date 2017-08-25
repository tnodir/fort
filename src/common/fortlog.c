/* Fort Firewall Log */

#define FORT_BUFFER_SIZE	16 * 1024
#define FORT_LOG_PATH_MAX	512
#define FORT_LOG_ALIGN		4

#define FORT_LOG_SIZE(path_len) \
  ((sizeof(UINT32) + sizeof(UINT32) + sizeof(UINT32) + (path_len)) \
     + (FORT_LOG_ALIGN-1)) & ~(FORT_LOG_ALIGN-1)


static void
fort_log_write (char *p, UINT32 remote_ip, UINT32 pid,
                UINT32 path_len, const char *path)
{
  UINT32 *up = (UINT32 *) p;

  *up++ = remote_ip;
  *up++ = pid;
  *up++ = path_len;

  p = (char *) up;

  if (path_len) {
    RtlCopyMemory(p, path, path_len);
  }
}

static void
fort_log_read (const char *p, UINT32 *remote_ip, UINT32 *pid,
               UINT32 *path_len, const char **path)
{
  UINT32 *up = (UINT32 *) p;

  *remote_ip = *up++;
  *pid = *up++;
  *path_len = *up++;

  p = (char *) up;

  *path = *path_len ? p : NULL;
}

