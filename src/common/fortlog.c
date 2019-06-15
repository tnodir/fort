/* Fort Firewall Driver Log */

#define FORT_BUFFER_SIZE		(16 * 1024 - 64)
#define FORT_LOG_PATH_MAX		512
#define FORT_LOG_ALIGN			4

#define FORT_LOG_FLAG_BLOCKED		0x01000000
#define FORT_LOG_FLAG_PROC_NEW		0x02000000
#define FORT_LOG_FLAG_STAT_TRAF		0x04000000
#define FORT_LOG_FLAG_TYPE_MASK		0xFF000000

#define FORT_LOG_BLOCKED_HEADER_SIZE	(4 * sizeof(UINT32))

#define FORT_LOG_BLOCKED_SIZE(path_len) \
  ((FORT_LOG_BLOCKED_HEADER_SIZE + (path_len) \
    + (FORT_LOG_ALIGN - 1)) & ~(FORT_LOG_ALIGN - 1))

#define FORT_LOG_BLOCKED_SIZE_MAX	FORT_LOG_BLOCKED_SIZE(FORT_LOG_PATH_MAX)

#define FORT_LOG_PROC_NEW_HEADER_SIZE	(2 * sizeof(UINT32))

#define FORT_LOG_PROC_NEW_SIZE(path_len) \
  ((FORT_LOG_PROC_NEW_HEADER_SIZE + (path_len) \
    + (FORT_LOG_ALIGN - 1)) & ~(FORT_LOG_ALIGN - 1))

#define FORT_LOG_STAT_HEADER_SIZE	sizeof(UINT32)

#define FORT_LOG_STAT_TRAF_SIZE(proc_count) \
  (proc_count * 3 * sizeof(UINT32))

#define FORT_LOG_STAT_SIZE(proc_count) \
  (FORT_LOG_STAT_HEADER_SIZE + FORT_LOG_STAT_TRAF_SIZE(proc_count))

#define FORT_LOG_STAT_BUFFER_PROC_COUNT \
  ((FORT_BUFFER_SIZE - FORT_LOG_STAT_HEADER_SIZE) / FORT_LOG_STAT_TRAF_SIZE(1))

#define FORT_LOG_SIZE_MAX	FORT_LOG_BLOCKED_SIZE_MAX

#define fort_log_type(p)	(*((UINT32 *) (p)) & FORT_LOG_FLAG_TYPE_MASK)


static void
fort_log_blocked_header_write (char *p, UINT32 remote_ip,
                               UINT16 remote_port, UCHAR ip_proto,
                               UINT32 pid, UINT32 path_len)
{
  UINT32 *up = (UINT32 *) p;

  *up++ = FORT_LOG_FLAG_BLOCKED | path_len;
  *up++ = remote_ip;
  *up++ = remote_port | (ip_proto << 16);
  *up = pid;
}

static void
fort_log_blocked_write (char *p, UINT32 remote_ip,
                        UINT16 remote_port, UCHAR ip_proto,
                        UINT32 pid, UINT32 path_len, const char *path)
{
  fort_log_blocked_header_write(p, remote_ip, remote_port,
    ip_proto, pid, path_len);

  if (path_len) {
    RtlCopyMemory(p + FORT_LOG_BLOCKED_HEADER_SIZE, path, path_len);
  }
}

static void
fort_log_blocked_header_read (const char *p, UINT32 *remote_ip,
                              UINT16 *remote_port, UCHAR *ip_proto,
                              UINT32 *pid, UINT32 *path_len)
{
  const UINT32 *up = (const UINT32 *) p;

  *path_len = (*up++ & ~FORT_LOG_FLAG_TYPE_MASK);
  *remote_ip = *up++;
  *remote_port = *((const UINT16 *) up);
  *ip_proto = (UCHAR) (*up++ >> 16);
  *pid = *up;
}

static void
fort_log_proc_new_header_write (char *p, UINT32 pid,
                                UINT32 path_len)
{
  UINT32 *up = (UINT32 *) p;

  *up++ = FORT_LOG_FLAG_PROC_NEW | path_len;
  *up = pid;
}

static void
fort_log_proc_new_write (char *p, UINT32 pid,
                         UINT32 path_len, const char *path)
{
  fort_log_proc_new_header_write(p, pid, path_len);

  if (path_len) {
    RtlCopyMemory(p + FORT_LOG_PROC_NEW_HEADER_SIZE, path, path_len);
  }
}

static void
fort_log_proc_new_header_read (const char *p, UINT32 *pid,
                               UINT32 *path_len)
{
  const UINT32 *up = (const UINT32 *) p;

  *path_len = (*up++ & ~FORT_LOG_FLAG_TYPE_MASK);
  *pid = *up;
}

static void
fort_log_stat_traf_header_write (char *p, UINT16 proc_count)
{
  UINT32 *up = (UINT32 *) p;

  *up++ = FORT_LOG_FLAG_STAT_TRAF | proc_count;
}

static void
fort_log_stat_traf_header_read (const char *p, UINT16 *proc_count)
{
  const UINT32 *up = (const UINT32 *) p;

  *proc_count = (UINT16) *up;
}
