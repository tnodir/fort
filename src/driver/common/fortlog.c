/* Fort Firewall Driver Log */

#include "fortlog.h"

#include "fortdef.h"

FORT_API void fort_log_blocked_header_write(char *p, BOOL blocked, UINT32 pid, UINT32 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(blocked ? FORT_LOG_TYPE_BLOCKED : FORT_LOG_TYPE_ALLOWED) | path_len;
    *up = pid;
}

FORT_API void fort_log_blocked_write(
        char *p, BOOL blocked, UINT32 pid, UINT32 path_len, const char *path)
{
    fort_log_blocked_header_write(p, blocked, pid, path_len);

    if (path_len != 0) {
        RtlCopyMemory(p + FORT_LOG_BLOCKED_HEADER_SIZE, path, path_len);
    }
}

FORT_API void fort_log_blocked_header_read(
        const char *p, BOOL *blocked, UINT32 *pid, UINT32 *path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    *blocked = (fort_log_type(up) == FORT_LOG_TYPE_BLOCKED);
    *path_len = (*up++ & ~FORT_LOG_FLAG_EX_MASK);
    *pid = *up;
}

void fort_log_blocked_ip_header_write(char *p, BOOL isIPv6, BOOL inbound, BOOL inherited,
        UCHAR block_reason, UCHAR ip_proto, UINT16 local_port, UINT16 remote_port,
        const UINT32 *local_ip, const UINT32 *remote_ip, UINT32 pid, UINT32 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_BLOCKED_IP) | (isIPv6 ? FORT_LOG_FLAG_IP6 : 0)
            | (inbound ? FORT_LOG_FLAG_IP_INBOUND : 0) | path_len;
    *up++ = (inherited ? FORT_LOG_BLOCKED_IP_INHERITED : 0) | ((UINT32) block_reason << 8)
            | ((UINT32) ip_proto << 16);
    *up++ = local_port | ((UINT32) remote_port << 16);
    *up++ = pid;

    const int ip_size = FORT_IP_ADDR_SIZE(isIPv6);
    RtlCopyMemory(up, local_ip, ip_size);

    up = (UINT32 *) ((PCHAR) up + ip_size);
    RtlCopyMemory(up, remote_ip, ip_size);
}

void fort_log_blocked_ip_write(char *p, BOOL isIPv6, BOOL inbound, BOOL inherited,
        UCHAR block_reason, UCHAR ip_proto, UINT16 local_port, UINT16 remote_port,
        const UINT32 *local_ip, const UINT32 *remote_ip, UINT32 pid, UINT32 path_len,
        const char *path)
{
    fort_log_blocked_ip_header_write(p, isIPv6, inbound, inherited, block_reason, ip_proto,
            local_port, remote_port, local_ip, remote_ip, pid, path_len);

    if (path_len != 0) {
        RtlCopyMemory(p + FORT_LOG_BLOCKED_IP_HEADER_SIZE(isIPv6), path, path_len);
    }
}

void fort_log_blocked_ip_header_read(const char *p, BOOL *isIPv6, BOOL *inbound, BOOL *inherited,
        UCHAR *block_reason, UCHAR *ip_proto, UINT16 *local_port, UINT16 *remote_port,
        UINT32 *local_ip, UINT32 *remote_ip, UINT32 *pid, UINT32 *path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    *isIPv6 = (*up & FORT_LOG_FLAG_IP6) != 0;
    *inbound = (*up & FORT_LOG_FLAG_IP_INBOUND) != 0;
    *path_len = (*up++ & ~FORT_LOG_FLAG_EX_MASK);

    const UCHAR flags = (UCHAR) *up;
    *inherited = (flags & FORT_LOG_BLOCKED_IP_INHERITED) != 0;
    *block_reason = (UCHAR) (*up >> 8);
    *ip_proto = (UCHAR) (*up++ >> 16);
    *local_port = *((const UINT16 *) up);
    *remote_port = (UINT16) (*up++ >> 16);
    *pid = *up++;

    const int ip_size = FORT_IP_ADDR_SIZE(*isIPv6);
    RtlCopyMemory(local_ip, up, ip_size);

    up = (const UINT32 *) ((const PCHAR) up + ip_size);
    RtlCopyMemory(remote_ip, up, ip_size);
}

FORT_API void fort_log_proc_new_header_write(char *p, UINT32 pid, UINT32 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_PROC_NEW) | path_len;
    *up = pid;
}

FORT_API void fort_log_proc_new_write(char *p, UINT32 pid, UINT32 path_len, const char *path)
{
    fort_log_proc_new_header_write(p, pid, path_len);

    if (path_len != 0) {
        RtlCopyMemory(p + FORT_LOG_PROC_NEW_HEADER_SIZE, path, path_len);
    }
}

FORT_API void fort_log_proc_new_header_read(const char *p, UINT32 *pid, UINT32 *path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    *path_len = (*up++ & ~FORT_LOG_FLAG_EX_MASK);
    *pid = *up;
}

FORT_API void fort_log_stat_traf_header_write(char *p, UINT16 proc_count)
{
    UINT32 *up = (UINT32 *) p;

    *up = fort_log_flag_type(FORT_LOG_TYPE_STAT_TRAF) | proc_count;
}

FORT_API void fort_log_stat_traf_header_read(const char *p, UINT16 *proc_count)
{
    const UINT32 *up = (const UINT32 *) p;

    *proc_count = (UINT16) *up;
}

FORT_API void fort_log_time_write(char *p, BOOL system_time_changed, INT64 unix_time)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_TIME) | (UCHAR) system_time_changed;
    *((INT64 *) up) = unix_time;
}

FORT_API void fort_log_time_read(const char *p, BOOL *system_time_changed, INT64 *unix_time)
{
    const UINT32 *up = (const UINT32 *) p;

    *system_time_changed = ((UCHAR) *up++ != 0);
    *unix_time = *((INT64 *) up);
}
