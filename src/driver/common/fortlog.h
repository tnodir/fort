#ifndef FORTLOG_H
#define FORTLOG_H

#include "common.h"

#define FORT_BUFFER_SIZE  (16 * 1024 - 64)
#define FORT_LOG_PATH_MAX 512
#define FORT_LOG_ALIGN    4

#define FORT_LOG_FLAG_BLOCKED       0x01000000
#define FORT_LOG_FLAG_PROC_NEW      0x02000000
#define FORT_LOG_FLAG_STAT_TRAF     0x04000000
#define FORT_LOG_FLAG_TIME          0x08000000
#define FORT_LOG_FLAG_BLOCKED_ALLOW 0x10000000
#define FORT_LOG_FLAG_TYPE_MASK     0x0F000000
#define FORT_LOG_FLAG_OPT_MASK      0xF0000000
#define FORT_LOG_FLAG_EX_MASK       0xFF000000

#define FORT_LOG_BLOCKED_HEADER_SIZE (4 * sizeof(UINT32))

#define FORT_LOG_BLOCKED_SIZE(path_len)                                                            \
    ((FORT_LOG_BLOCKED_HEADER_SIZE + (path_len) + (FORT_LOG_ALIGN - 1)) & ~(FORT_LOG_ALIGN - 1))

#define FORT_LOG_BLOCKED_SIZE_MAX FORT_LOG_BLOCKED_SIZE(FORT_LOG_PATH_MAX)

#define FORT_LOG_PROC_NEW_HEADER_SIZE (2 * sizeof(UINT32))

#define FORT_LOG_PROC_NEW_SIZE(path_len)                                                           \
    ((FORT_LOG_PROC_NEW_HEADER_SIZE + (path_len) + (FORT_LOG_ALIGN - 1)) & ~(FORT_LOG_ALIGN - 1))

#define FORT_LOG_STAT_HEADER_SIZE (sizeof(UINT32))

#define FORT_LOG_STAT_TRAF_SIZE(proc_count) (proc_count * 3 * sizeof(UINT32))

#define FORT_LOG_STAT_SIZE(proc_count)                                                             \
    (FORT_LOG_STAT_HEADER_SIZE + FORT_LOG_STAT_TRAF_SIZE(proc_count))

#define FORT_LOG_STAT_BUFFER_PROC_COUNT                                                            \
    ((FORT_BUFFER_SIZE - FORT_LOG_STAT_HEADER_SIZE) / FORT_LOG_STAT_TRAF_SIZE(1))

#define FORT_LOG_TIME_SIZE (sizeof(UINT32) + sizeof(INT64))

#define FORT_LOG_SIZE_MAX FORT_LOG_BLOCKED_SIZE_MAX

#define fort_log_type(p) (*((UINT32 *) (p)) & FORT_LOG_FLAG_TYPE_MASK)

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_log_blocked_header_write(char *p, BOOL blocked, UINT32 remote_ip,
        UINT16 remote_port, UCHAR ip_proto, UINT32 pid, UINT32 path_len);

FORT_API void fort_log_blocked_write(char *p, BOOL blocked, UINT32 remote_ip, UINT16 remote_port,
        UCHAR ip_proto, UINT32 pid, UINT32 path_len, const char *path);

FORT_API void fort_log_blocked_header_read(const char *p, BOOL *blocked, UINT32 *remote_ip,
        UINT16 *remote_port, UCHAR *ip_proto, UINT32 *pid, UINT32 *path_len);

FORT_API void fort_log_proc_new_header_write(char *p, UINT32 pid, UINT32 path_len);

FORT_API void fort_log_proc_new_write(char *p, UINT32 pid, UINT32 path_len, const char *path);

FORT_API void fort_log_proc_new_header_read(const char *p, UINT32 *pid, UINT32 *path_len);

FORT_API void fort_log_stat_traf_header_write(char *p, UINT16 proc_count);

FORT_API void fort_log_stat_traf_header_read(const char *p, UINT16 *proc_count);

FORT_API void fort_log_time_write(char *p, INT64 unix_time);

FORT_API void fort_log_time_read(const char *p, INT64 *unix_time);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTLOG_H
