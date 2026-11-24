/* Fort Firewall Driver Log */

#include "fortlog.h"

static void fort_log_path_write(char *p, PCFORT_APP_PATH path)
{
    const UINT16 path_len = path->len;

    if (path_len != 0) {
        RtlCopyMemory(p, path->buffer, path_len);
    }

    /* Zero the alignment's padding, which is copied to user mode */
    RtlZeroMemory(p + path_len, FORT_ALIGN_SIZE(path_len, FORT_LOG_ALIGN) - path_len);
}

/* The flow's id by its low and high halves */
static UINT32 *fort_log_u64_write(UINT32 *up, UINT64 v)
{
    *up++ = (UINT32) v;
    *up++ = (UINT32) (v >> 32);

    return up;
}

static const UINT32 *fort_log_u64_read(const UINT32 *up, UINT64 *v)
{
    *v = *up++;
    *v |= ((UINT64) *up++) << 32;

    return up;
}

FORT_API void fort_log_app_header_write(
        char *p, const FORT_APP_FLAGS app_flags, UINT32 pid, UINT16 path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_APP)
            | (app_flags.blocked ? FORT_LOG_FLAG_OPT_BLOCKED : 0)
            | (app_flags.alerted ? FORT_LOG_FLAG_OPT_ALERTED : 0) | path_len;
    *up = pid;
}

FORT_API void fort_log_app_write(
        char *p, const FORT_APP_FLAGS app_flags, UINT32 pid, PCFORT_APP_PATH path)
{
    const UINT16 path_len = path->len;

    fort_log_app_header_write(p, app_flags, pid, path_len);

    fort_log_path_write(p + FORT_LOG_APP_HEADER_SIZE, path);
}

FORT_API void fort_log_app_header_read(
        const char *p, PFORT_APP_FLAGS app_flags, UINT32 *pid, UINT16 *path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    app_flags->blocked = (*up & FORT_LOG_FLAG_OPT_BLOCKED) != 0;
    app_flags->alerted = (*up & FORT_LOG_FLAG_OPT_ALERTED) != 0;
    *path_len = (UINT16) (*up++ & ~FORT_LOG_FLAG_EX_MASK);
    *pid = *up;
}

inline static UINT32 fort_log_conn_proc_flags(PCFORT_CONF_META_CONN conn)
{
    return (conn->inherited ? FORT_LOG_CONN_INHERITED : 0)
            | (conn->app_asked ? FORT_LOG_CONN_APP_ASKED : 0)
            | (conn->proc_logged ? FORT_LOG_CONN_PROC_PATH : 0);
}

inline static UINT32 fort_log_conn_net_flags(PCFORT_CONF_META_CONN conn)
{
    return (conn->isIPv6 ? FORT_LOG_CONN_IP6 : 0) | (conn->inbound ? FORT_LOG_CONN_INBOUND : 0)
            | (conn->is_loopback ? FORT_LOG_CONN_LOOPBACK : 0)
            | (conn->is_local_net ? FORT_LOG_CONN_LOCAL_NET : 0);
}

inline static UINT32 fort_log_conn_flags(PCFORT_CONF_META_CONN conn)
{
    return fort_log_conn_net_flags(conn) | (conn->act.conn_alert ? FORT_LOG_CONN_ALERTED : 0)
            | (conn->act.terminated ? FORT_LOG_CONN_TERMINATED : 0)
            | (conn->flow_logging ? FORT_LOG_CONN_FLOW_LOGGING : 0)
            | (conn->activity_only ? FORT_LOG_CONN_ACTIVITY : 0) | fort_log_conn_proc_flags(conn);
}

FORT_API void fort_log_conn_header_write(
        char *p, PCFORT_CONF_META_CONN conn, UINT16 path_len, UINT16 inherit_path_len)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_CONN)
            | (conn->act.blocked ? FORT_LOG_FLAG_OPT_BLOCKED : 0)
            | (conn->act.drop_blocked ? FORT_LOG_FLAG_OPT_DROPPED : 0) | fort_log_conn_flags(conn);
    *up++ = conn->reason | ((UINT32) conn->ip_proto << 8);
    *up++ = ((UINT32) conn->rule_id) | ((UINT32) conn->act.zone_id << 16);
    *up++ = conn->local_port | ((UINT32) conn->remote_port << 16);
    *up++ = conn->app.data.app_id;
    *up++ = conn->process_id;
    up = fort_log_u64_write(up, conn->flow_id);
    *up++ = path_len | ((UINT32) inherit_path_len << 16);

    const int ip_size = FORT_IP_ADDR_SIZE(conn->isIPv6);

    // Local IP
    RtlCopyMemory(up, conn->local_ip.data, ip_size);

    // Remote IP
    up = (UINT32 *) ((PCHAR) up + ip_size);
    RtlCopyMemory(up, conn->remote_ip.data, ip_size);
}

FORT_API void fort_log_conn_write(
        char *p, PCFORT_CONF_META_CONN conn, PCFORT_APP_PATH path, PCFORT_APP_PATH inherit_path)
{
    const UINT16 path_len = path->len;
    const BOOL isIPv6 = conn->isIPv6;

    fort_log_conn_header_write(p, conn, path_len, inherit_path->len);

    fort_log_path_write(p + FORT_LOG_CONN_HEADER_SIZE(isIPv6), path);
    fort_log_path_write(p + FORT_LOG_CONN_INHERIT_PATH_OFFSET(path_len, isIPv6), inherit_path);
}

FORT_API void fort_log_conn_header_read(
        const char *p, PFORT_CONF_META_CONN conn, UINT16 *path_len, UINT16 *inherit_path_len)
{
    const UINT32 *up = (const UINT32 *) p;

    UINT32 v;
    v = *up++;
    conn->act.blocked = (v & FORT_LOG_FLAG_OPT_BLOCKED) != 0;
    conn->act.drop_blocked = (v & FORT_LOG_FLAG_OPT_DROPPED) != 0;
    conn->isIPv6 = (v & FORT_LOG_CONN_IP6) != 0;
    conn->inbound = (v & FORT_LOG_CONN_INBOUND) != 0;
    conn->is_loopback = (v & FORT_LOG_CONN_LOOPBACK) != 0;
    conn->is_local_net = (v & FORT_LOG_CONN_LOCAL_NET) != 0;
    conn->inherited = (v & FORT_LOG_CONN_INHERITED) != 0;
    conn->act.conn_alert = (v & FORT_LOG_CONN_ALERTED) != 0;
    conn->act.terminated = (v & FORT_LOG_CONN_TERMINATED) != 0;
    conn->app_asked = (v & FORT_LOG_CONN_APP_ASKED) != 0;
    conn->proc_logged = (v & FORT_LOG_CONN_PROC_PATH) != 0;
    conn->flow_logging = (v & FORT_LOG_CONN_FLOW_LOGGING) != 0;
    conn->activity_only = (v & FORT_LOG_CONN_ACTIVITY) != 0;

    v = *up++;
    conn->reason = (UCHAR) v;
    conn->ip_proto = (UCHAR) (v >> 8);

    v = *up++;
    conn->act.zone_id = (UCHAR) (v >> 16);
    conn->rule_id = (UINT16) v;

    v = *up++;
    conn->local_port = (UINT16) v;
    conn->remote_port = (UINT16) (v >> 16);

    v = *up++;
    conn->app.data.app_id = v;

    v = *up++;
    conn->process_id = v;

    up = fort_log_u64_read(up, &conn->flow_id);

    v = *up++;
    *path_len = (UINT16) v;
    *inherit_path_len = (UINT16) (v >> 16);

    const int ip_size = FORT_IP_ADDR_SIZE(conn->isIPv6);

    // Local IP
    RtlCopyMemory(conn->local_ip.data, up, ip_size);

    // Remote IP
    up = (const UINT32 *) ((const PCHAR) up + ip_size);
    RtlCopyMemory(conn->remote_ip.data, up, ip_size);
}

/* Returns the output after the process's header */
static UINT32 *fort_log_proc_header_write(char *p, UINT32 type, PCFORT_LOG_PROC_HEADER header)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(type) | header->path_len;
    *up++ = header->process_id;
    *up++ = header->parent_process_id;

    return fort_log_u64_write(up, (UINT64) header->create_time);
}

FORT_API void fort_log_proc_start_header_write(char *p, PCFORT_LOG_PROC_HEADER header)
{
    fort_log_proc_header_write(p, FORT_LOG_TYPE_PROC_START, header);
}

FORT_API void fort_log_proc_start_write(
        char *p, PCFORT_LOG_PROC_HEADER header, PCFORT_APP_PATH path)
{
    fort_log_proc_start_header_write(p, header);

    fort_log_path_write(p + FORT_LOG_PROC_HEADER_SIZE, path);
}

FORT_API void fort_log_proc_header_read(const char *p, PFORT_LOG_PROC_HEADER header)
{
    const UINT32 *up = (const UINT32 *) p;

    header->path_len = (UINT16) (*up++ & ~FORT_LOG_FLAG_EX_MASK);
    header->process_id = *up++;
    header->parent_process_id = *up++;

    fort_log_u64_read(up, (UINT64 *) &header->create_time);
}

FORT_API void fort_log_proc_new_header_write(char *p, PCFORT_LOG_PROC_NEW_HEADER header)
{
    UINT32 *up = fort_log_proc_header_write(p, FORT_LOG_TYPE_PROC_NEW, &header->proc);

    *up++ = header->app_id;
    *up = header->inherit_path_len;
}

FORT_API void fort_log_proc_new_write(
        char *p, PCFORT_CONF_META_CONN conn, PCFORT_APP_PATH path, PCFORT_APP_PATH inherit_path)
{
    const FORT_LOG_PROC_NEW_HEADER header = {
        .inherit_path_len = inherit_path->len,
        .app_id = conn->app.data.app_id,
        .proc = {
            .path_len = path->len,
            .process_id = conn->process_id,
            .parent_process_id = conn->parent_process_id,
            .create_time = conn->create_time,
        },
    };

    fort_log_proc_new_header_write(p, &header);

    fort_log_path_write(p + FORT_LOG_PROC_NEW_HEADER_SIZE, path);
    fort_log_path_write(p + FORT_LOG_PROC_NEW_INHERIT_PATH_OFFSET(path->len), inherit_path);
}

FORT_API void fort_log_proc_new_header_read(const char *p, PFORT_LOG_PROC_NEW_HEADER header)
{
    fort_log_proc_header_read(p, &header->proc);

    const UINT32 *up = (const UINT32 *) (p + FORT_LOG_PROC_HEADER_SIZE);

    header->app_id = *up++;
    header->inherit_path_len = (UINT16) *up;
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

FORT_API void fort_log_time_write(char *p, INT64 unix_time, BOOL system_time_changed)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_TIME) | (UCHAR) system_time_changed;
    *((INT64 *) up) = unix_time;
}

FORT_API void fort_log_time_read(const char *p, INT64 *unix_time, BOOL *system_time_changed)
{
    const UINT32 *up = (const UINT32 *) p;

    *system_time_changed = ((UCHAR) *up++ != 0);
    *unix_time = *((INT64 *) up);
}

FORT_API void fort_log_proc_pid_write(char *p, UINT32 type, UINT32 pid)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(type);
    *up = pid;
}

FORT_API void fort_log_proc_pid_read(const char *p, UINT32 *pid)
{
    const UINT32 *up = (const UINT32 *) p;

    up++;
    *pid = *up;
}

FORT_API void fort_log_flow_traf_header_write(char *p, UINT16 flow_count)
{
    UINT32 *up = (UINT32 *) p;

    *up = fort_log_flag_type(FORT_LOG_TYPE_FLOW_TRAF) | flow_count;
}

FORT_API void fort_log_flow_traf_header_read(const char *p, UINT16 *flow_count)
{
    const UINT32 *up = (const UINT32 *) p;

    *flow_count = (UINT16) *up;
}

FORT_API char *fort_log_flow_stat_write(char *p, PCFORT_FLOW_STAT flow_stat)
{
    UINT32 *up = (UINT32 *) p;

    up = fort_log_u64_write(up, flow_stat->flow_id);
    *up++ = flow_stat->traf.in_bytes;
    *up++ = flow_stat->traf.out_bytes;

    return (char *) up;
}

FORT_API void fort_log_flow_stat_read(const char *p, PFORT_FLOW_STAT flow_stat)
{
    const UINT32 *up = (const UINT32 *) p;

    up = fort_log_u64_read(up, &flow_stat->flow_id);
    flow_stat->traf.in_bytes = *up++;
    flow_stat->traf.out_bytes = *up;
}

FORT_API void fort_log_flow_closed_write(char *p, PCFORT_FLOW_STAT flow_stat)
{
    UINT32 *up = (UINT32 *) p;

    *up++ = fort_log_flag_type(FORT_LOG_TYPE_FLOW_CLOSED);

    fort_log_flow_stat_write((char *) up, flow_stat);
}

FORT_API void fort_log_flow_closed_read(const char *p, PFORT_FLOW_STAT flow_stat)
{
    fort_log_flow_stat_read(p + sizeof(UINT32), flow_stat);
}
