#ifndef FORTLOG_H
#define FORTLOG_H

#include "common.h"

#include "fortconf.h"

#define FORT_BUFFER_SIZE  (16 * 1024 - 64)
#define FORT_LOG_PATH_MAX (2 * 1024 * sizeof(WCHAR))
#define FORT_LOG_ALIGN    4

#define FORT_LOG_FLAG_TYPE_MASK     0x0FF00000
#define FORT_LOG_FLAG_TYPE_MASK_OFF 20
#define FORT_LOG_FLAG_OPT_BLOCKED   0x10000000
#define FORT_LOG_FLAG_OPT_MASK      0xF0000000
#define FORT_LOG_FLAG_EX_MASK       (FORT_LOG_FLAG_TYPE_MASK | FORT_LOG_FLAG_OPT_MASK)

#define fort_log_flag_type(type) (((UINT32) (type)) << FORT_LOG_FLAG_TYPE_MASK_OFF)

#define fort_log_type(p)                                                                           \
    ((*((UINT32 *) (p)) & FORT_LOG_FLAG_TYPE_MASK) >> FORT_LOG_FLAG_TYPE_MASK_OFF)

#define FORT_LOG_APP_HEADER_SIZE (2 * sizeof(UINT32))

#define FORT_LOG_APP_SIZE(path_len)                                                                \
    FORT_ALIGN_SIZE(FORT_LOG_APP_HEADER_SIZE + (path_len), FORT_LOG_ALIGN)

#define FORT_IP_ADDR_SIZE(isIPv6) ((isIPv6) ? sizeof(ip6_addr_t) : sizeof(UINT32))

#define FORT_LOG_CONN_HEADER_SIZE(isIPv6) (5 * sizeof(UINT32) + 2 * FORT_IP_ADDR_SIZE(isIPv6))

#define FORT_LOG_CONN_SIZE(path_len, isIPv6)                                                       \
    FORT_ALIGN_SIZE(FORT_LOG_CONN_HEADER_SIZE(isIPv6) + (path_len), FORT_LOG_ALIGN)

#define FORT_LOG_PROC_NEW_HEADER_SIZE (2 * sizeof(UINT32))

#define FORT_LOG_PROC_NEW_SIZE(path_len)                                                           \
    FORT_ALIGN_SIZE(FORT_LOG_PROC_NEW_HEADER_SIZE + (path_len), FORT_LOG_ALIGN)

#define FORT_LOG_STAT_HEADER_SIZE (sizeof(UINT32))

#define FORT_LOG_STAT_TRAF_SIZE(proc_count) ((proc_count) * sizeof(UINT32) * 3)

#define FORT_LOG_STAT_SIZE(proc_count)                                                             \
    (FORT_LOG_STAT_HEADER_SIZE + FORT_LOG_STAT_TRAF_SIZE(proc_count))

#define FORT_LOG_STAT_BUFFER_PROC_COUNT                                                            \
    ((FORT_BUFFER_SIZE - FORT_LOG_STAT_HEADER_SIZE) / FORT_LOG_STAT_TRAF_SIZE(1))

#define FORT_LOG_TIME_SIZE (sizeof(UINT32) + sizeof(INT64))

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_log_app_header_write(char *p, BOOL blocked, UINT32 pid, UINT16 path_len);

FORT_API void fort_log_app_write(char *p, BOOL blocked, UINT32 pid, PCFORT_APP_PATH path);

FORT_API void fort_log_app_header_read(const char *p, BOOL *blocked, UINT32 *pid, UINT16 *path_len);

FORT_API void fort_log_conn_header_write(char *p, PCFORT_CONF_META_CONN conn, UINT16 path_len);

FORT_API void fort_log_conn_write(char *p, PCFORT_CONF_META_CONN conn, PCFORT_APP_PATH path);

FORT_API void fort_log_conn_header_read(const char *p, PFORT_CONF_META_CONN conn, UINT16 *path_len);

FORT_API void fort_log_proc_new_header_write(char *p, UINT32 pid, UINT16 path_len);

FORT_API void fort_log_proc_new_write(char *p, UINT32 pid, PCFORT_APP_PATH path);

FORT_API void fort_log_proc_new_header_read(const char *p, UINT32 *pid, UINT16 *path_len);

FORT_API void fort_log_stat_traf_header_write(char *p, UINT16 proc_count);

FORT_API void fort_log_stat_traf_header_read(const char *p, UINT16 *proc_count);

FORT_API void fort_log_time_write(char *p, BOOL system_time_changed, INT64 unix_time);

FORT_API void fort_log_time_read(const char *p, BOOL *system_time_changed, INT64 *unix_time);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTLOG_H
