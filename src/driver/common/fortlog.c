/* Fort Firewall Driver Log */

#include "fortlog.h"

#include "fortdef.h"

FORT_API void fort_log_app_header_write(char *p, BOOL blocked, UINT32 pid, UINT16 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_APP) | (blocked ? FORT_LOG_FLAG_OPT_BLOCKED : 0)
            | path_len;
    *up = pid;
}

FORT_API void fort_log_app_write(char *p, BOOL blocked, UINT32 pid, PCFORT_APP_PATH path)
{
    const UINT16 path_len = path->len;

    fort_log_app_header_write(p, blocked, pid, path_len);

    if (path_len != 0) {
        RtlCopyMemory(p + FORT_LOG_APP_HEADER_SIZE, path->buffer, path_len);
    }
}

FORT_API void fort_log_app_header_read(const char *p, BOOL *blocked, UINT32 *pid, UINT16 *path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    *blocked = (*up & FORT_LOG_FLAG_OPT_BLOCKED) != 0;
    *path_len = (UINT16) (*up++ & ~FORT_LOG_FLAG_EX_MASK);
    *pid = *up;
}

FORT_API void fort_log_conn_header_write(char *p, PCFORT_CONF_META_CONN conn, UINT16 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_CONN) | (conn->blocked ? FORT_LOG_FLAG_OPT_BLOCKED : 0)
            | path_len;
    *up++ = (conn->isIPv6 ? FORT_LOG_CONN_IP6 : 0) | (conn->inbound ? FORT_LOG_CONN_INBOUND : 0)
            | (conn->inherited ? FORT_LOG_CONN_INHERITED : 0) | ((UINT32) conn->reason << 8)
            | ((UINT32) conn->ip_proto << 16);
    *up++ = ((UINT32) conn->rule_id) | ((UINT32) conn->zone_id << 16);
    *up++ = conn->local_port | ((UINT32) conn->remote_port << 16);
    *up++ = conn->process_id;

    const int ip_size = FORT_IP_ADDR_SIZE(conn->isIPv6);

    // Local IP
    RtlCopyMemory(up, conn->local_ip.data, ip_size);

    // Remote IP
    up = (UINT32 *) ((PCHAR) up + ip_size);
    RtlCopyMemory(up, conn->remote_ip.data, ip_size);
}

FORT_API void fort_log_conn_write(char *p, PCFORT_CONF_META_CONN conn, PCFORT_APP_PATH path)
{
    const UINT16 path_len = path->len;

    fort_log_conn_header_write(p, conn, path_len);

    if (path_len != 0) {
        RtlCopyMemory(p + FORT_LOG_CONN_HEADER_SIZE(conn->isIPv6), path->buffer, path_len);
    }
}

FORT_API void fort_log_conn_header_read(const char *p, PFORT_CONF_META_CONN conn, UINT16 *path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    UINT32 v;
    v = *up++;
    conn->blocked = (v & FORT_LOG_FLAG_OPT_BLOCKED) != 0;
    *path_len = (v & ~FORT_LOG_FLAG_EX_MASK);

    v = *up++;
    const UCHAR flags = (UCHAR) v;
    conn->isIPv6 = (flags & FORT_LOG_CONN_IP6) != 0;
    conn->inbound = (flags & FORT_LOG_CONN_INBOUND) != 0;
    conn->inherited = (flags & FORT_LOG_CONN_INHERITED) != 0;
    conn->reason = (UCHAR) (v >> 8);
    conn->ip_proto = (UCHAR) (v >> 16);

    v = *up++;
    conn->zone_id = (UCHAR) (v >> 16);
    conn->rule_id = (UINT16) v;

    v = *up++;
    conn->local_port = (UINT16) v;
    conn->remote_port = (UINT16) (v >> 16);

    v = *up++;
    conn->process_id = v;

    const int ip_size = FORT_IP_ADDR_SIZE(conn->isIPv6);

    // Local IP
    RtlCopyMemory(conn->local_ip.data, up, ip_size);

    // Remote IP
    up = (const UINT32 *) ((const PCHAR) up + ip_size);
    RtlCopyMemory(conn->remote_ip.data, up, ip_size);
}

FORT_API void fort_log_proc_new_header_write(char *p, UINT32 pid, UINT16 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_PROC_NEW) | path_len;
    *up = pid;
}

FORT_API void fort_log_proc_new_write(char *p, UINT32 pid, PCFORT_APP_PATH path)
{
    const UINT16 path_len = path->len;

    fort_log_proc_new_header_write(p, pid, path_len);

    if (path_len != 0) {
        RtlCopyMemory(p + FORT_LOG_PROC_NEW_HEADER_SIZE, path->buffer, path_len);
    }
}

FORT_API void fort_log_proc_new_header_read(const char *p, UINT32 *pid, UINT16 *path_len)
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
