/* Fort Firewall Pending Packets */

#include "fortpkt_pending.h"

#include "common/fortconf_conn.h"

#include "fortcnf_conf.h"
#include "fortdev.h"
#include "fortthr.h"
#include "fortutl.h"

typedef BOOL fort_pending_packet_done_func(void *ctx, PCFORT_PENDING_PACKET pkt);

typedef struct fort_pending_walk
{
    fort_pending_packet_done_func *done_func;
    void *ctx; /* the done_func's context */

    PFORT_PENDING_PACKET packets; /* the done packets */
} FORT_PENDING_WALK, *PFORT_PENDING_WALK;

typedef struct fort_pending_reauth_ctx
{
    FORT_CONF_FLAGS conf_flags;

    PFORT_CONF_REF conf_ref;
} FORT_PENDING_REAUTH_CTX, *PFORT_PENDING_REAUTH_CTX;

typedef const FORT_PENDING_REAUTH_CTX *PCFORT_PENDING_REAUTH_CTX;

typedef struct fort_pending_app_state
{
    UINT16 inbound_count : 8;
    UINT16 outbound_count : 7;
    UINT16 flow_pended : 1; /* the connection's repeated packet */
} FORT_PENDING_APP_STATE, *PFORT_PENDING_APP_STATE;

typedef struct fort_pending_flows_ctx
{
    UINT16 count;

    const UINT64 *flow_ids;
} FORT_PENDING_FLOWS_CTX, *PFORT_PENDING_FLOWS_CTX;

typedef const FORT_PENDING_FLOWS_CTX *PCFORT_PENDING_FLOWS_CTX;

static PFORT_PENDING_PACKET fort_pending_packet_new(PCFORT_CONF_META_CONN conn)
{
    /* The process's name, if any, is found by the connection's path */
    PFORT_PSNAME_NODE ps_name = fort_psname_get(&fort_device()->ps_names, &conn->path);
    if (ps_name == NULL)
        return NULL;

    PFORT_PENDING_PACKET pkt = fort_mem_alloc(sizeof(FORT_PENDING_PACKET), FORT_PACKET_POOL_TAG);
    if (pkt == NULL) {
        fort_psname_put(&fort_device()->ps_names, ps_name);
        return NULL;
    }

    RtlZeroMemory(pkt, sizeof(FORT_PENDING_PACKET));

    pkt->ps_name = ps_name;

    return pkt;
}

inline static void fort_pending_packet_del(PFORT_PENDING_PACKET pkt)
{
    fort_mem_free(pkt, FORT_PACKET_POOL_TAG);
}

FORT_API void fort_pending_packet_free(PFORT_PENDING_PACKET pkt)
{
    fort_packet_free(&pkt->io);

    fort_psname_put(&fort_device()->ps_names, pkt->ps_name);

    fort_pending_packet_del(pkt);
}

static void fort_pending_app_state_add(
        PFORT_PENDING_APP_STATE state, PCFORT_PENDING_PACKET pkt, UINT64 flow_id)
{
    state->flow_pended |= (pkt->flow_id == flow_id);

    if (pkt->inbound) {
        state->inbound_count++;
    } else {
        state->outbound_count++;
    }
}

/* The App's packet: by its interned name, else by its path */
inline static BOOL fort_pending_packet_app(
        PCFORT_PENDING_PACKET pkt, PCFORT_CONF_META_CONN conn, PFORT_PSNAME_NODE ps_name)
{
    /* The equal interned names are the same one */
    if (ps_name != NULL) {
        return pkt->ps_name == ps_name;
    }

    return fort_psname_equal(pkt->ps_name, &conn->path);
}

static FORT_PENDING_APP_STATE fort_pending_app_state_locked(
        PFORT_PENDING pending, PCFORT_CONF_META_CONN conn, PFORT_PSNAME_NODE ps_name)
{
    FORT_PENDING_APP_STATE state = { 0 };

    PCFORT_PENDING_PACKET pkt = pending->packets_head;

    for (; pkt != NULL; pkt = pkt->next) {
        if (fort_pending_packet_app(pkt, conn, ps_name)) {
            fort_pending_app_state_add(&state, pkt, conn->flow_id);
        }
    }

    return state;
}

static BOOL fort_pending_conn_pended_locked(
        PFORT_PENDING pending, PCFORT_CONF_META_CONN conn, PFORT_PSNAME_NODE ps_name)
{
    const FORT_PENDING_APP_STATE state = fort_pending_app_state_locked(pending, conn, ps_name);

    if (state.flow_pended)
        return TRUE;

    /* Check the App's Limits */
    return conn->inbound ? state.inbound_count >= FORT_PENDING_APP_INBOUND_COUNT_MAX
                         : state.outbound_count >= FORT_PENDING_APP_OUTBOUND_COUNT_MAX;
}

static NTSTATUS fort_pending_add_packet_locked(PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca,
        PCFORT_CONF_META_CONN conn, PFORT_PENDING_PACKET pkt)
{
    /* The closing device clears the flag before the pending packets: the one added later isn't
     * decided by the next master */
    if (fort_device_flag(&fort_device()->conf, FORT_DEVICE_IS_OPENED) == 0)
        return STATUS_DEVICE_NOT_CONNECTED;

    /* Check the Limits */
    if (pending->packet_count >= FORT_PENDING_PACKET_COUNT_MAX)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (fort_pending_conn_pended_locked(pending, conn, pkt->ps_name))
        return STATUS_ALREADY_REGISTERED;

    const NTSTATUS status =
            FwpsPendOperation0(ca->inMetaValues->completionHandle, &pkt->completion_context);
    if (!NT_SUCCESS(status))
        return status;

    pending->packet_count++;

    pkt->next = pending->packets_head;
    pending->packets_head = pkt;

    return STATUS_SUCCESS;
}

static NTSTATUS fort_pending_add_packet_list(PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca,
        PCFORT_CONF_META_CONN conn, PFORT_PENDING_PACKET pkt)
{
    NTSTATUS status;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    status = fort_pending_add_packet_locked(pending, ca, conn, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return status;
}

static void fort_pending_walk_locked(PFORT_PENDING pending, PFORT_PENDING_WALK walk)
{
    PFORT_PENDING_PACKET *pkt_link = &pending->packets_head;

    while (*pkt_link != NULL) {
        PFORT_PENDING_PACKET pkt = *pkt_link;

        if (!walk->done_func(walk->ctx, pkt)) {
            pkt_link = &pkt->next;
            continue;
        }

        /* Move the done packet */
        *pkt_link = pkt->next;
        pending->packet_count--;

        pkt->next = walk->packets;
        walk->packets = pkt;
    }
}

static void fort_pending_walk(PFORT_PENDING pending, PFORT_PENDING_WALK walk)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    fort_pending_walk_locked(pending, walk);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static BOOL fort_pending_packet_inject(PFORT_PENDING_PACKET pkt)
{
    if (pkt->io.netBufList == NULL)
        return FALSE; /* not cloned */

    return NT_SUCCESS(fort_packet_inject(&pkt->io));
}

static void fort_pending_packets_complete(PFORT_PENDING_PACKET pkt, BOOL inject)
{
    while (pkt != NULL) {
        PFORT_PENDING_PACKET next = pkt->next;

        /* Reauthorize the connection */
        FwpsCompleteOperation0(pkt->completion_context, NULL);

        /* Re-inject the cloned packet, it's checked again */
        if (!(inject && fort_pending_packet_inject(pkt))) {
            fort_pending_packet_free(pkt);
        }

        pkt = next;
    }
}

static void fort_pending_complete_walk(PFORT_PENDING pending, PFORT_PENDING_WALK walk, BOOL reauth)
{
    fort_pending_walk(pending, walk);

    /* Re-inject the reauthorized ones' packets, block the others (e.g. timed out) */
    fort_pending_packets_complete(walk->packets, /*inject=*/reauth);
}

FORT_API void fort_pending_open(PFORT_PENDING pending)
{
    FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport4_in_id);
    FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport4_out_id);
    FwpsInjectionHandleCreate0(
            AF_INET6, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport6_in_id);
    FwpsInjectionHandleCreate0(
            AF_INET6, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport6_out_id);

    KeInitializeSpinLock(&pending->lock);
}

static void fort_pending_wait_injections(PFORT_PENDING pending)
{
    for (;;) {
        const LONG inject_count = InterlockedOr(&pending->inject_count, 0);

        fort_thread_delay(/*msecs=*/10);

        if (inject_count == 0)
            break; /* Check the extra one time to ensure the completion's exit */
    }
}

FORT_API void fort_pending_close(PFORT_PENDING pending)
{
    /* Complete the pending connections */
    fort_pending_clear(pending);

    /* Wait for the asynchronous injections before their handles' destruction */
    fort_pending_wait_injections(pending);

    FwpsInjectionHandleDestroy0(pending->injection_transport4_in_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport4_out_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport6_in_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport6_out_id);
}

static BOOL fort_pending_packet_any(void *ctx, PCFORT_PENDING_PACKET pkt)
{
    UNUSED(ctx);
    UNUSED(pkt);

    return TRUE;
}

FORT_API void fort_pending_clear(PFORT_PENDING pending)
{
    FORT_PENDING_WALK walk = {
        .done_func = &fort_pending_packet_any,
    };

    /* Complete the connections without re-injecting their packets */
    fort_pending_complete_walk(pending, &walk, /*reauth=*/FALSE);
}

static BOOL fort_pending_packet_listed(void *ctx, PCFORT_PENDING_PACKET pkt)
{
    PCFORT_PENDING_FLOWS_CTX fc = ctx;

    return fort_pending_flow_index(fc->flow_ids, fc->count, pkt->flow_id) < fc->count;
}

FORT_API void fort_pending_complete_flows(
        PFORT_PENDING pending, const UINT64 *flow_ids, UINT16 count, BOOL reauth)
{
    FORT_PENDING_FLOWS_CTX fc = {
        .count = count,
        .flow_ids = flow_ids,
    };

    FORT_PENDING_WALK walk = {
        .done_func = &fort_pending_packet_listed,
        .ctx = &fc,
    };

    fort_pending_complete_walk(pending, &walk, reauth);
}

FORT_API BOOL fort_pending_conn_pended(PFORT_PENDING pending, PCFORT_CONF_META_CONN conn)
{
    BOOL pended;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    /* The App's name isn't interned yet */
    pended = fort_pending_conn_pended_locked(pending, conn, /*ps_name=*/NULL);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pended;
}

static BOOL fort_pending_packet_decided(void *ctx, PCFORT_PENDING_PACKET pkt)
{
    PCFORT_PENDING_REAUTH_CTX rc = ctx;

    PFORT_CONF_REF conf_ref = rc->conf_ref;

    /* The closed device blocks them by the clearing */
    if (conf_ref == NULL)
        return FALSE;

    if (!rc->conf_flags.filter_enabled)
        return TRUE;

    const FORT_APP_PATH path = fort_psname_path(pkt->ps_name);

    const FORT_CONF_APP_FIND_RESULT app =
            fort_conf_app_find(&conf_ref->conf, &path, fort_conf_exe_find, conf_ref);

    return !fort_conf_app_asked(rc->conf_flags, app.data);
}

FORT_API void fort_pending_reauth(PFORT_PENDING pending)
{
    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;

    FORT_PENDING_REAUTH_CTX rc = {
        .conf_flags = fort_device_conf_flags(device_conf),
        .conf_ref = fort_conf_ref_take(device_conf),
    };

    FORT_PENDING_WALK walk = {
        .done_func = &fort_pending_packet_decided,
        .ctx = &rc,
    };

    /* Reauthorize the decided connections */
    fort_pending_complete_walk(pending, &walk, /*reauth=*/TRUE);

    if (rc.conf_ref != NULL) {
        fort_conf_ref_put(device_conf, rc.conf_ref);
    }
}

FORT_API NTSTATUS fort_pending_add_packet(
        PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn)
{
    NTSTATUS status;

    if (!FWPS_IS_METADATA_FIELD_PRESENT(ca->inMetaValues, FWPS_METADATA_FIELD_COMPLETION_HANDLE))
        return STATUS_NOT_SUPPORTED;

    /* Create the Packet */
    PFORT_PENDING_PACKET pkt = fort_pending_packet_new(conn);
    if (pkt == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    pkt->app_asked = (UCHAR) conn->app_asked;
    pkt->inbound = (UCHAR) conn->inbound;
    pkt->flow_id = conn->flow_id;

    const UCHAR ipsec_flag = fort_packet_is_ipsec_protected(ca) ? FORT_PACKET_IPSEC_PROTECTED : 0;

    status = fort_packet_fill(ca, &pkt->io, ipsec_flag | FORT_PACKET_TYPE_PENDING);
    if (NT_SUCCESS(status)) {
        /* Add the Packet to the list */
        status = fort_pending_add_packet_list(pending, ca, conn, pkt);
    }

    if (!NT_SUCCESS(status)) {
        fort_pending_packet_free(pkt);
    }

    return status;
}
