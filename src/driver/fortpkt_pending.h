#ifndef FORTPKT_PENDING_H
#define FORTPKT_PENDING_H

#include "fortpkt.h"
#include "fortpsname.h"

typedef struct fort_pending_packet
{
    FORT_PACKET_IO io; /* must be first! */

    struct fort_pending_packet *next;

    UCHAR inbound : 1;
    UCHAR app_asked : 1; /* asked by the App's data, else by a Rule: decided by the user only */
    UCHAR unlogged : 1; /* not shown to the user: blocked by the worker */

    UINT64 flow_id; /* the connection's id to decide it by the user or the timeout */

    HANDLE completion_context;

    PFORT_PSNAME_NODE ps_name; /* the App's referenced path for its limits */
} FORT_PENDING_PACKET, *PFORT_PENDING_PACKET;

typedef const FORT_PENDING_PACKET *PCFORT_PENDING_PACKET;

#define FORT_PENDING_PACKET_COUNT_MAX 1024

/* An App's limits: the inbound connection is decided by the filters only, one is enough to show */
#define FORT_PENDING_APP_INBOUND_COUNT_MAX  1
#define FORT_PENDING_APP_OUTBOUND_COUNT_MAX 9

#define FORT_PENDING_FLOW_COUNT_MAX 0xFFFF /* the flows' lists by the 16-bit count */

/* The allowed connection's reauthorization follows its completion in milliseconds: the not taken
 * one (e.g. closed meanwhile) is dropped after the seconds by the next allowing */
#define FORT_PENDING_ALLOWED_FLOW_SECONDS 10

/* The connection allowed once by the user */
typedef struct fort_pending_allowed_flow
{
    UINT64 flow_id; /* must be first: the binary search by it */

    INT64 expire_time; /* the interrupt time */
} FORT_PENDING_ALLOWED_FLOW, *PFORT_PENDING_ALLOWED_FLOW;

typedef const FORT_PENDING_ALLOWED_FLOW *PCFORT_PENDING_ALLOWED_FLOW;

typedef struct fort_pending
{
    HANDLE injection_transport4_in_id;
    HANDLE injection_transport4_out_id;
    HANDLE injection_transport6_in_id;
    HANDLE injection_transport6_out_id;

    LONG volatile inject_count; /* asynchronous injections in progress */

    UINT16 packet_count;

    UINT16 volatile allowed_flow_count; /* checked without the lock: usually empty */

    /* The connections allowed once by the user: sorted, till their reauthorization */
    PFORT_PENDING_ALLOWED_FLOW allowed_flows;

    PFORT_PENDING_PACKET packets_head;

    KSPIN_LOCK lock;
} FORT_PENDING, *PFORT_PENDING;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_pending_packet_free(PFORT_PENDING_PACKET pkt);

FORT_API void fort_pending_open(PFORT_PENDING pending);

FORT_API void fort_pending_close(PFORT_PENDING pending);

FORT_API void fort_pending_clear(PFORT_PENDING pending);

/* The flow ids are sorted by the master */
FORT_API void fort_pending_complete_flows(
        PFORT_PENDING pending, const UINT64 *flow_ids, UINT16 count, BOOL reauth);

/* The flow ids are sorted by the master */
FORT_API NTSTATUS fort_pending_allow_flows(
        PFORT_PENDING pending, const UINT64 *flow_ids, UINT16 count);

/* The allowed connection's first reauthorization takes it: then its associated flow keeps it */
FORT_API BOOL fort_pending_flow_take_allowed(PFORT_PENDING pending, UINT64 flow_id);

FORT_API BOOL fort_pending_conn_pended(PFORT_PENDING pending, PCFORT_CONF_META_CONN conn);

FORT_API void fort_pending_reauth(PFORT_PENDING pending);

FORT_API BOOL fort_pending_conn_unlogged(PFORT_PENDING pending, PCFORT_CONF_META_CONN conn);

FORT_API void fort_pending_block_unlogged(PFORT_PENDING pending);

/* STATUS_ALREADY_REGISTERED: the connection's repeated packet or the App's one over its limit */
FORT_API NTSTATUS fort_pending_add_packet(
        PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPKT_PENDING_H
