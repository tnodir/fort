/* Fort Firewall Driver Log */

#include "fortlog.h"

FORT_API void fort_log_blocked_header_write(char *p, BOOL blocked, UINT32 remote_ip,
    UINT16 remote_port, UCHAR ip_proto, UINT32 pid, UINT32 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = FORT_LOG_FLAG_BLOCKED | (blocked ? 0 : FORT_LOG_FLAG_BLOCKED_ALLOW) | path_len;
    *up++ = remote_ip;
    *up++ = remote_port | (ip_proto << 16);
    *up = pid;
}

FORT_API void fort_log_blocked_write(char *p, BOOL blocked, UINT32 remote_ip, UINT16 remote_port,
    UCHAR ip_proto, UINT32 pid, UINT32 path_len, const char *path)
{
    fort_log_blocked_header_write(p, blocked, remote_ip, remote_port, ip_proto, pid, path_len);

    if (path_len) {
        RtlCopyMemory(p + FORT_LOG_BLOCKED_HEADER_SIZE, path, path_len);
    }
}

FORT_API void fort_log_blocked_header_read(const char *p, BOOL *blocked, UINT32 *remote_ip,
    UINT16 *remote_port, UCHAR *ip_proto, UINT32 *pid, UINT32 *path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    *blocked = !(*up & FORT_LOG_FLAG_BLOCKED_ALLOW);
    *path_len = (*up++ & ~FORT_LOG_FLAG_EX_MASK);
    *remote_ip = *up++;
    *remote_port = *((const UINT16 *) up);
    *ip_proto = (UCHAR)(*up++ >> 16);
    *pid = *up;
}

FORT_API void fort_log_proc_new_header_write(char *p, UINT32 pid, UINT32 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = FORT_LOG_FLAG_PROC_NEW | path_len;
    *up = pid;
}

FORT_API void fort_log_proc_new_write(char *p, UINT32 pid, UINT32 path_len, const char *path)
{
    fort_log_proc_new_header_write(p, pid, path_len);

    if (path_len) {
        RtlCopyMemory(p + FORT_LOG_PROC_NEW_HEADER_SIZE, path, path_len);
    }
}

FORT_API void fort_log_proc_new_header_read(const char *p, UINT32 *pid, UINT32 *path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    *path_len = (*up++ & ~FORT_LOG_FLAG_EX_MASK);
    *pid = *up;
}

FORT_API void fort_log_stat_traf_header_write(char *p, INT64 unix_time, UINT16 proc_count)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = FORT_LOG_FLAG_STAT_TRAF | proc_count;
    *((INT64 *) up) = unix_time;
}

FORT_API void fort_log_stat_traf_header_read(const char *p, INT64 *unix_time, UINT16 *proc_count)
{
    const UINT32 *up = (const UINT32 *) p;

    *proc_count = (UINT16) *up++;
    *unix_time = *((INT64 *) up);
}
