#ifndef FORTBUF_H
#define FORTBUF_H

#include "fortdrv.h"

#include "common/fortlog.h"

typedef struct fort_buffer_data
{
    struct fort_buffer_data *next;

    UINT32 top;
    CHAR p[FORT_BUFFER_SIZE];
} FORT_BUFFER_DATA, *PFORT_BUFFER_DATA;

typedef struct fort_buffer
{
    PFORT_BUFFER_DATA data_head;
    PFORT_BUFFER_DATA data_tail; /* last is current */
    PFORT_BUFFER_DATA data_free;

    PIRP irp; /* pending */
    PCHAR out;
    ULONG out_len;
    UINT32 out_top;

    KSPIN_LOCK lock;
} FORT_BUFFER, *PFORT_BUFFER;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_buffer_open(PFORT_BUFFER buf);

FORT_API void fort_buffer_close(PFORT_BUFFER buf);

FORT_API void fort_buffer_clear(PFORT_BUFFER buf);

FORT_API NTSTATUS fort_buffer_prepare(
        PFORT_BUFFER buf, UINT32 len, PCHAR *out, PIRP *irp, ULONG_PTR *info);

FORT_API NTSTATUS fort_buffer_blocked_write(PFORT_BUFFER buf, BOOL blocked, UINT32 pid,
        UINT32 path_len, const PVOID path, PIRP *irp, ULONG_PTR *info);

FORT_API NTSTATUS fort_buffer_blocked_ip_write(PFORT_BUFFER buf, BOOL isIPv6, BOOL inbound,
        BOOL inherited, UCHAR block_reason, UCHAR ip_proto, UINT16 local_port, UINT16 remote_port,
        const UINT32 *local_ip, const UINT32 *remote_ip, UINT32 pid, UINT32 path_len,
        const PVOID path, PIRP *irp, ULONG_PTR *info);

FORT_API NTSTATUS fort_buffer_proc_new_write(PFORT_BUFFER buf, UINT32 pid, UINT32 path_len,
        const PVOID path, PIRP *irp, ULONG_PTR *info);

FORT_API NTSTATUS fort_buffer_xmove(
        PFORT_BUFFER buf, PIRP irp, PVOID out, ULONG out_len, ULONG_PTR *info);

FORT_API void fort_buffer_irp_mark_pending(PIRP irp);

FORT_API void fort_buffer_irp_clear_pending(PIRP irp);

FORT_API void fort_buffer_dpc_begin(PFORT_BUFFER buf, PKLOCK_QUEUE_HANDLE lock_queue);

FORT_API void fort_buffer_dpc_end(PKLOCK_QUEUE_HANDLE lock_queue);

FORT_API void fort_buffer_flush_pending(PFORT_BUFFER buf, PIRP *irp, ULONG_PTR *info);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTBUF_H
