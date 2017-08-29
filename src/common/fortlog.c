/* Fort Firewall Driver Log */

#define FORT_BUFFER_SIZE	16 * 1024
#define FORT_LOG_PATH_MAX	512
#define FORT_LOG_ALIGN		4

#define FORT_LOG_HEADER_SIZE	(3 * (sizeof(UINT32)))

#define FORT_LOG_SIZE(path_len) \
  ((FORT_LOG_HEADER_SIZE + (path_len) \
    + (FORT_LOG_ALIGN - 1)) & ~(FORT_LOG_ALIGN - 1))


static void
fort_log_header_write (char *p, UINT32 remote_ip, UINT32 pid,
                       UINT32 path_len)
{
  UINT32 *up = (UINT32 *) p;

  *up++ = remote_ip;
  *up++ = pid;
  *up = path_len;
}

static void
fort_log_write (char *p, UINT32 remote_ip, UINT32 pid,
                UINT32 path_len, const char *path)
{
  fort_log_header_write(p, remote_ip, pid, path_len);

  if (path_len) {
    RtlCopyMemory(p + FORT_LOG_HEADER_SIZE, path, path_len);
  }
}

static void
fort_log_header_read (const char *p, UINT32 *remote_ip, UINT32 *pid,
                      UINT32 *path_len)
{
  UINT32 *up = (UINT32 *) p;

  *remote_ip = *up++;
  *pid = *up++;
  *path_len = *up;
}

static void
fort_log_read (const char *p, UINT32 *remote_ip, UINT32 *pid,
               UINT32 *path_len, const char **path)
{
  fort_log_header_read(p, remote_ip, pid, path_len);

  *path = p + FORT_LOG_HEADER_SIZE;
}

