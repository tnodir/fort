#ifndef FORTPKT_H
#define FORTPKT_H

#include "fortdrv.h"

#include "common/fortconf.h"
#include "forttds.h"

#define FORT_DEFER_FLUSH_ALL 0xFFFFFFFF
#define FORT_DEFER_LIST_MAX  (FORT_CONF_GROUP_MAX * 2)

#define FORT_DEFER_STREAM_ALL ((UINT64)((INT64) -1))

#define TCP_FLAG_FIN 0x0001
#define TCP_FLAG_SYN 0x0002
#define TCP_FLAG_RST 0x0004
#define TCP_FLAG_PSH 0x0008
#define TCP_FLAG_ACK 0x0010
#define TCP_FLAG_URG 0x0020
#define TCP_FLAG_ECE 0x0040
#define TCP_FLAG_CWR 0x0080

typedef struct tcp_header
{
    UINT16 source; /* Source Port */
    UINT16 dest; /* Destination Port */

    UINT32 seq; /* Sequence number */
    UINT32 ack_seq; /* Acknowledgement number */

    UCHAR res1 : 4; /* Unused */
    UCHAR doff : 4; /* Data offset */

    UCHAR flags; /* Flags */

    UINT16 window; /* Window size */
    UINT16 csum; /* Checksum */
    UINT16 urg_ptr; /* Urgent Pointer */
} TCP_HEADER, *PTCP_HEADER;

typedef struct fort_packet_in
{
    COMPARTMENT_ID compartmentId;

    IF_INDEX interfaceIndex;
    IF_INDEX subInterfaceIndex;

    UINT16 ipHeaderSize;
    UINT16 transportHeaderSize;
} FORT_PACKET_IN, *PFORT_PACKET_IN;

typedef struct fort_packet_out
{
    COMPARTMENT_ID compartmentId;

    UINT32 remoteAddr4;
    SCOPE_ID remoteScopeId;

    UINT64 endpointHandle;
} FORT_PACKET_OUT, *PFORT_PACKET_OUT;

typedef struct fort_packet_stream
{
    UINT16 layerId;

    UINT32 streamFlags;
    UINT32 calloutId;

    UINT64 flow_id;
} FORT_PACKET_STREAM, *PFORT_PACKET_STREAM;

typedef struct fort_packet
{
    UINT32 inbound : 1;
    UINT32 is_stream : 1;
    UINT32 dataOffset : 12;
    UINT32 dataSize : 18;

    PNET_BUFFER_LIST netBufList;

    struct fort_packet *next;

    union {
        FORT_PACKET_IN in;
        FORT_PACKET_OUT out;
        FORT_PACKET_STREAM stream;
    };
} FORT_PACKET, *PFORT_PACKET;

typedef struct fort_defer_list
{
    PFORT_PACKET packet_head;
    PFORT_PACKET packet_tail;
} FORT_DEFER_LIST, *PFORT_DEFER_LIST;

typedef struct fort_defer
{
    UINT32 list_bits;

    HANDLE transport_injection4_id;
    HANDLE stream_injection4_id;

    PFORT_PACKET packet_free;

    tommy_arrayof packets;

    FORT_DEFER_LIST stream_list;
    FORT_DEFER_LIST lists[FORT_DEFER_LIST_MAX]; /* in/out-bounds */

    KSPIN_LOCK lock;
} FORT_DEFER, *PFORT_DEFER;

typedef void (NTAPI *FORT_INJECT_COMPLETE_FUNC)(PFORT_PACKET, PNET_BUFFER_LIST, BOOLEAN);
typedef NTSTATUS (*FORT_INJECT_FUNC)(
        PFORT_DEFER, PFORT_PACKET, PNET_BUFFER_LIST *, FORT_INJECT_COMPLETE_FUNC);

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_defer_open(PFORT_DEFER defer);

FORT_API void fort_defer_close(PFORT_DEFER defer);

FORT_API NTSTATUS fort_defer_packet_add(PFORT_DEFER defer,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        BOOL inbound, UCHAR group_index);

FORT_API NTSTATUS fort_defer_stream_add(PFORT_DEFER defer,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const FWPS_STREAM_DATA0 *streamData,
        const FWPS_FILTER0 *filter, BOOL inbound);

FORT_API void fort_defer_packet_free(
        PFORT_DEFER defer, PFORT_PACKET pkt, PNET_BUFFER_LIST clonedNetBufList, BOOL dispatchLevel);

FORT_API void fort_defer_packet_flush(PFORT_DEFER defer, FORT_INJECT_COMPLETE_FUNC complete_func,
        UINT32 list_bits, BOOL dispatchLevel);

FORT_API void fort_defer_stream_flush(PFORT_DEFER defer, FORT_INJECT_COMPLETE_FUNC complete_func,
        UINT64 flow_id, BOOL dispatchLevel);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPKT_H
