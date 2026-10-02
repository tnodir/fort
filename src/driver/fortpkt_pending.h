#ifndef FORTPKT_PENDING_H
#define FORTPKT_PENDING_H

#include "fortpkt.h"

#include "forttds.h"

typedef struct fort_pending_packet
{
    FORT_PACKET_IO io; /* must be first! */

    struct fort_pending_packet *next;

    HANDLE completion_context;
} FORT_PENDING_PACKET, *PFORT_PENDING_PACKET;

#define FORT_PENDING_PROC_COUNT_MAX        1024
#define FORT_PENDING_PROC_PACKET_COUNT_MAX 3

typedef struct fort_pending_proc
{
    struct fort_pending_proc *next;

    PFORT_PENDING_PACKET packets_head;

    UINT16 packet_count;

    UINT32 process_id;
} FORT_PENDING_PROC, *PFORT_PENDING_PROC;

typedef struct fort_pending
{
    HANDLE injection_transport4_in_id;
    HANDLE injection_transport4_out_id;
    HANDLE injection_transport6_in_id;
    HANDLE injection_transport6_out_id;

    LONG volatile inject_count; /* asynchronous injections in progress */

    UINT16 proc_count;

    PFORT_PENDING_PROC proc_free;
    tommy_arrayof procs;

    PFORT_PENDING_PROC procs_head;

    KSPIN_LOCK lock;
} FORT_PENDING, *PFORT_PENDING;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_pending_packet_free(PFORT_PENDING_PACKET pkt);

FORT_API void fort_pending_open(PFORT_PENDING pending);

FORT_API void fort_pending_close(PFORT_PENDING pending);

FORT_API void fort_pending_clear(PFORT_PENDING pending);

FORT_API BOOL fort_pending_add_packet(
        PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPKT_PENDING_H
