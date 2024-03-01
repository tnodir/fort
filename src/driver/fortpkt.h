#ifndef FORTPKT_H
#define FORTPKT_H

#include "fortdrv.h"

#include "common/fortconf.h"
#include "fortcoutarg.h"
#include "forttds.h"
#include "fortthr.h"

#define FORT_PACKET_QUEUE_BAD_INDEX ((UINT16) -1)

typedef struct fort_packet_in
{
    IF_INDEX interfaceIndex;
    IF_INDEX subInterfaceIndex;
} FORT_PACKET_IN, *PFORT_PACKET_IN;

typedef struct fort_packet_out
{
    WSACMSGHDR *controlData;
    ULONG controlDataLength;

    SCOPE_ID remoteScopeId;
    UINT64 endpointHandle;

    ip_addr_t remoteAddr;
} FORT_PACKET_OUT, *PFORT_PACKET_OUT;

#define FORT_PACKET_INBOUND         0x01
#define FORT_PACKET_IP6             0x02
#define FORT_PACKET_IPSEC_PROTECTED 0x08
#define FORT_PACKET_TYPE_FLOW       0x10
#define FORT_PACKET_TYPE_PENDING    0x20
#define FORT_PACKET_TYPE_MASK       0x30

typedef struct fort_packet_io
{
    UCHAR flags;

    /* Data for re-injection */
    COMPARTMENT_ID compartmentId;
    PNET_BUFFER_LIST netBufList;
    union {
        FORT_PACKET_IN in;
        FORT_PACKET_OUT out;
    };
} FORT_PACKET_IO, *PFORT_PACKET_IO;

typedef struct fort_flow_packet
{
    FORT_PACKET_IO io; /* must be first! */

    struct fort_flow_packet *next;

    PVOID flow; /* to drop on flow deletion */

    LARGE_INTEGER latency_start; /* Time it was placed in the latency queue */
    UINT32 data_length; /* Size of the packet (in bytes) */
} FORT_FLOW_PACKET, *PFORT_FLOW_PACKET;

typedef struct fort_packet_list
{
    PFORT_FLOW_PACKET packet_head;
    PFORT_FLOW_PACKET packet_tail;
} FORT_PACKET_LIST, *PFORT_PACKET_LIST;

typedef struct fort_packet_queue
{
    /* All packets are first buffered into the bandwidth queue and released
     * at the appropriate rate for the configured bandwidth into the latency queue.
     * When they are added to the latency queue they are timestamped when they
     * entered and they are released when the appropriate latency has expired.
     * Only the bandwidth queue is affected by the queue buffer size.
     * The latency queue has no limit.
     */
    FORT_PACKET_LIST bandwidth_list;
    FORT_PACKET_LIST latency_list;

    FORT_SPEED_LIMIT limit;

    UINT64 queued_bytes; /* accumulated size of queued packets */
    UINT64 available_bytes; /* accumulated bytes available for sending */
    LARGE_INTEGER last_tick; /* last time the queue was checked */

    KSPIN_LOCK lock;
} FORT_PACKET_QUEUE, *PFORT_PACKET_QUEUE;

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

    UINT16 proc_count;

    PFORT_PENDING_PROC proc_free;
    tommy_arrayof procs;

    PFORT_PENDING_PROC procs_head;

    KSPIN_LOCK lock;
} FORT_PENDING, *PFORT_PENDING;

#define FORT_SHAPER_CLOSED 0x01

typedef struct fort_shaper
{
    UCHAR volatile flags;

    UINT32 limit_io_bits;

    LONG volatile group_io_bits;
    LONG volatile active_io_bits;

    ULONG randomSeed;
    LARGE_INTEGER qpcFrequency;

    KEVENT thread_event;
    FORT_THREAD thread;

    KSPIN_LOCK lock;

    PFORT_PACKET_QUEUE queues[FORT_CONF_GROUP_MAX * 2]; /* in/out-bound pairs */
} FORT_SHAPER, *PFORT_SHAPER;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_shaper_open(PFORT_SHAPER shaper);

FORT_API void fort_shaper_close(PFORT_SHAPER shaper);

FORT_API void fort_shaper_conf_update(PFORT_SHAPER shaper, const PFORT_CONF_IO conf_io);

FORT_API void fort_shaper_conf_flags_update(PFORT_SHAPER shaper, const PFORT_CONF_FLAGS conf_flags);

FORT_API BOOL fort_shaper_packet_process(PFORT_SHAPER shaper, PFORT_CALLOUT_ARG ca);

FORT_API void fort_shaper_drop_flow_packets(PFORT_SHAPER shaper, UINT64 flowContext);

FORT_API void fort_shaper_drop_packets(PFORT_SHAPER shaper);

FORT_API void fort_pending_open(PFORT_PENDING pending);

FORT_API void fort_pending_close(PFORT_PENDING pending);

FORT_API void fort_pending_clear(PFORT_PENDING pending);

FORT_API BOOL fort_pending_add_packet(
        PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPKT_H
