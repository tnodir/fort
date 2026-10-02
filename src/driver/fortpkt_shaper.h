#ifndef FORTPKT_SHAPER_H
#define FORTPKT_SHAPER_H

#include "fortpkt.h"

#include "fortthr.h"

#define FORT_PACKET_QUEUE_BAD_INDEX ((UINT16) - 1)

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
     * The latency queue has no limit, except the shaper's limit of held inbound packets.
     */
    FORT_PACKET_LIST bandwidth_list;
    FORT_PACKET_LIST latency_list;

    FORT_SPEED_LIMIT limit;

    UINT64 queued_bytes; /* accumulated size of queued packets */
    INT64 available_bytes; /* accumulated bytes available for sending, negative on debt */
    UINT64 available_rem; /* fractional part of the available bytes, multiplied by QPC frequency */
    LARGE_INTEGER last_tick; /* last time the queue was checked */

    KSPIN_LOCK lock;
} FORT_PACKET_QUEUE, *PFORT_PACKET_QUEUE;

#define FORT_SHAPER_CLOSED  0x01
#define FORT_SHAPER_ENABLED 0x02

typedef struct fort_shaper
{
    UCHAR volatile flags;

    UINT32 limit_bits; /* configured Speed Limits */
    UINT32 limit_enabled_bits;

    LONG volatile enabled_bits; /* shaped queues */
    LONG volatile active_io_bits;

    LONG volatile in_packet_count; /* held inbound packets */

    UINT32 randomSeed;
    LARGE_INTEGER qpcFrequency;

    KEVENT thread_event;
    FORT_THREAD thread;

    KSPIN_LOCK lock;

    PFORT_PACKET_QUEUE queues[FORT_CONF_SPEED_LIMIT_MAX]; /* by Speed Limit index */
} FORT_SHAPER, *PFORT_SHAPER;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_shaper_packet_free(PFORT_SHAPER shaper, PFORT_FLOW_PACKET pkt);

FORT_API void fort_shaper_open(PFORT_SHAPER shaper);

FORT_API void fort_shaper_close(PFORT_SHAPER shaper);

FORT_API void fort_shaper_speed_limits_set(
        PFORT_SHAPER shaper, PCFORT_CONF_SPEED_LIMITS speed_limits);

FORT_API void fort_shaper_speed_limit_flags_set(
        PFORT_SHAPER shaper, PCFORT_CONF_SPEED_LIMIT_FLAGS limit_flags);

FORT_API void fort_shaper_conf_flags_update(PFORT_SHAPER shaper, const FORT_CONF_FLAGS conf_flags);

FORT_API BOOL fort_shaper_packet_process(PFORT_SHAPER shaper, PFORT_CALLOUT_ARG ca);

FORT_API void fort_shaper_drop_flow_packets(PFORT_SHAPER shaper, UINT64 flowContext);

FORT_API void fort_shaper_drop_packets(PFORT_SHAPER shaper);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPKT_SHAPER_H
