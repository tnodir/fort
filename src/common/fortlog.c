/* Fort Firewall Driver Log */

#define FORT_BUFFER_SIZE		(16 * 1024)
#define FORT_LOG_PATH_MAX		512
#define FORT_LOG_ALIGN			4

#define FORT_LOG_FLAG_BLOCKED		0x01000000
#define FORT_LOG_FLAG_STAT		0x02000000
#define FORT_LOG_FLAG_TYPE_MASK		0xFF000000

#define FORT_LOG_BLOCKED_HEADER_SIZE	(3 * sizeof(UINT32))

#define FORT_LOG_BLOCKED_SIZE(path_len) \
  ((FORT_LOG_BLOCKED_HEADER_SIZE + (path_len) \
    + (FORT_LOG_ALIGN - 1)) & ~(FORT_LOG_ALIGN - 1))

#define fort_log_type(p)	(*((UINT32 *) (p)) & FORT_LOG_FLAG_TYPE_MASK)


static void
fort_log_blocked_header_write (char *p, UINT32 remote_ip, UINT32 pid,
                               UINT32 path_len)
{
  UINT32 *up = (UINT32 *) p;

  *up++ = FORT_LOG_FLAG_BLOCKED | path_len;
  *up++ = remote_ip;
  *up = pid;
}

static void
fort_log_blocked_write (char *p, UINT32 remote_ip, UINT32 pid,
                        UINT32 path_len, const char *path)
{
  fort_log_blocked_header_write(p, remote_ip, pid, path_len);

  if (path_len) {
    RtlCopyMemory(p + FORT_LOG_BLOCKED_HEADER_SIZE, path, path_len);
  }
}

static void
fort_log_blocked_header_read (const char *p, UINT32 *remote_ip, UINT32 *pid,
                              UINT32 *path_len)
{
  UINT32 *up = (UINT32 *) p;

  *path_len = (*up++ & ~FORT_LOG_FLAG_TYPE_MASK);
  *remote_ip = *up++;
  *pid = *up;
}

static void
fort_log_blocked_read (const char *p, UINT32 *remote_ip, UINT32 *pid,
                       UINT32 *path_len, const char **path)
{
  fort_log_blocked_header_read(p, remote_ip, pid, path_len);

  *path = p + FORT_LOG_BLOCKED_HEADER_SIZE;
}

