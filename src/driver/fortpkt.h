#ifndef FORTPKT_H
#define FORTPKT_H

#include "fortdrv.h"

#include "common/fortconf.h"
#include "fortstat.h"
#include "forttmr.h"

#define FORT_PACKET_FLUSH_ALL 0xFFFFFFFF

#define FORT_PACKET_QUEUE_BAD_INDEX ((UINT16) -1)

#define FORT_PACKET_INBOUND 0x01
#define FORT_PACKET_IP4     0x02
#define FORT_PACKET_IP6     0x04

#define FORT_MAC_FRAME_PACKET_COUNT_MAX 0xFF

typedef struct fort_packet
{
    struct fort_packet *next;

    LARGE_INTEGER latency_start; /* Time it was placed in the latency queue */
    UINT32 data_length; /* Size of the packet (in bytes) */

    UCHAR flags;

    /* Data for re-injection */
    UINT16 layerId;
    IF_INDEX interfaceIndex;
    NDIS_PORT_NUMBER ndisPortNumber;
    PNET_BUFFER_LIST netBufList;
} FORT_PACKET, *PFORT_PACKET;

typedef struct fort_packet_list
{
    PFORT_PACKET packet_head;
    PFORT_PACKET packet_tail;
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

typedef struct fort_shaper
{
    UINT32 group_io_bits;
    UINT32 limit_io_bits;

    ULONG volatile active_io_bits;

    HANDLE injection_ip4;
    HANDLE injection_ip6;
    HANDLE injection_unspec;

    FORT_TIMER timer;

    PFORT_PACKET_QUEUE queues[FORT_CONF_GROUP_MAX * 2]; /* in/out-bound pairs */

    KSPIN_LOCK lock;
} FORT_SHAPER, *PFORT_SHAPER;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_shaper_open(PFORT_SHAPER shaper);

FORT_API void fort_shaper_close(PFORT_SHAPER shaper);

FORT_API void fort_shaper_conf_update(PFORT_SHAPER shaper, const PFORT_CONF_IO conf_io);

FORT_API void fort_shaper_conf_flags_update(PFORT_SHAPER shaper, const PFORT_CONF_FLAGS conf_flags);

FORT_API BOOL fort_shaper_packet_process(PFORT_SHAPER shaper,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        BOOL inbound, UCHAR group_index);

FORT_API void fort_shaper_flush(PFORT_SHAPER shaper, UINT32 group_io_bits, BOOL drop);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPKT_H
