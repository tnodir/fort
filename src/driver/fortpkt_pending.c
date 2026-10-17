/* Fort Firewall Pending Packets */

#include "fortpkt_pending.h"

#include "common/fortconf_conn.h"
#include "common/fortdef.h"

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
    UINT16 allowed_count;
    UINT16 allowed_size; /* the allowed flows' new array's size */

    INT64 now; /* the interrupt time of the allowing */

    const UINT64 *flow_ids;
    PFORT_PENDING_ALLOWED_FLOW allowed_flows; /* the new array, NULL - not allowing */
} FORT_PENDING_FLOWS_CTX, *PFORT_PENDING_FLOWS_CTX;

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

/* Binary search of the items sorted by their first field, the flow's id: the flow's index or the
 * count, when not found */
static UINT32 fort_pending_flow_index(
        const void *items, UINT32 count, UINT32 item_size, UINT64 flow_id)
{
    UINT32 low = 0;
    UINT32 high = count;

    while (low < high) {
        const UINT32 mid = low + (high - low) / 2;
        const UINT64 id = *(const UINT64 *) ((const char *) items + mid * item_size);

        if (id == flow_id)
            return mid;

        if (id < flow_id) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return count;
}

/* Sorted insertion into the array with the room for it */
static UINT16 fort_pending_allowed_flow_insert(
        PFORT_PENDING_ALLOWED_FLOW flows, UINT16 count, const FORT_PENDING_ALLOWED_FLOW flow)
{
    UINT16 i = count;

    while (i > 0 && flows[i - 1].flow_id > flow.flow_id) {
        flows[i] = flows[i - 1];
        --i;
    }

    flows[i] = flow;

    return count + 1;
}

/* Copy the not expired ones */
static UINT16 fort_pending_allowed_flows_copy(PFORT_PENDING_ALLOWED_FLOW flows,
        PCFORT_PENDING_ALLOWED_FLOW old_flows, UINT16 old_count, INT64 now)
{
    UINT16 count = 0;

    for (UINT16 i = 0; i < old_count; ++i) {
        if (old_flows[i].expire_time > now) {
            flows[count++] = old_flows[i];
        }
    }

    return count;
}

static BOOL fort_pending_allowed_flow_remove_locked(PFORT_PENDING pending, UINT64 flow_id)
{
    PFORT_PENDING_ALLOWED_FLOW flows = pending->allowed_flows;
    const UINT32 count = pending->allowed_flow_count;

    const UINT32 index =
            fort_pending_flow_index(flows, count, sizeof(FORT_PENDING_ALLOWED_FLOW), flow_id);
    if (index >= count)
        return FALSE;

    RtlMoveMemory(flows + index, flows + index + 1,
            (count - index - 1) * sizeof(FORT_PENDING_ALLOWED_FLOW));

    pending->allowed_flow_count = (UINT16) (count - 1);

    return TRUE;
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

    /* The closed allowed flow's id may be reused by the new flow */
    fort_pending_allowed_flow_remove_locked(pending, pkt->flow_id);

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

static void fort_pending_allowed_flows_clear(PFORT_PENDING pending)
{
    PFORT_PENDING_ALLOWED_FLOW flows;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    flows = pending->allowed_flows;

    pending->allowed_flows = NULL;
    pending->allowed_flow_count = 0;

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    if (flows != NULL) {
        fort_mem_free(flows, FORT_PACKET_POOL_TAG);
    }
}

FORT_API void fort_pending_clear(PFORT_PENDING pending)
{
    FORT_PENDING_WALK walk = {
        .done_func = &fort_pending_packet_any,
    };

    /* Complete the connections without re-injecting their packets */
    fort_pending_complete_walk(pending, &walk, /*reauth=*/FALSE);

    /* The closed master's allowed connections */
    fort_pending_allowed_flows_clear(pending);
}

static BOOL fort_pending_packet_listed(void *ctx, PCFORT_PENDING_PACKET pkt)
{
    PFORT_PENDING_FLOWS_CTX fc = ctx;

    if (fort_pending_flow_index(fc->flow_ids, fc->count, sizeof(UINT64), pkt->flow_id) >= fc->count)
        return FALSE;

    /* The allowed connection's reauthorization finds it */
    if (fc->allowed_flows != NULL) {
        const FORT_PENDING_ALLOWED_FLOW flow = {
            .flow_id = pkt->flow_id,
            .expire_time = fc->now + (INT64) FORT_PENDING_ALLOWED_FLOW_SECONDS * 10000000,
        };

        fc->allowed_count =
                fort_pending_allowed_flow_insert(fc->allowed_flows, fc->allowed_count, flow);
    }

    return TRUE;
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

/* Returns the array to free: the old allowed flows' one, else the new one without the room */
static PFORT_PENDING_ALLOWED_FLOW fort_pending_allow_walk_locked(
        PFORT_PENDING pending, PFORT_PENDING_WALK walk)
{
    PFORT_PENDING_FLOWS_CTX fc = walk->ctx;

    PFORT_PENDING_ALLOWED_FLOW old_flows = pending->allowed_flows;
    const UINT16 old_count = pending->allowed_flow_count;

    /* The allowed flows are added meanwhile */
    if (old_count + fc->count > fc->allowed_size) {
        return fc->allowed_flows;
    }

    /* The not taken ones are dropped by their expiration */
    fc->allowed_count =
            fort_pending_allowed_flows_copy(fc->allowed_flows, old_flows, old_count, fc->now);

    fort_pending_walk_locked(pending, walk);

    pending->allowed_flows = fc->allowed_flows;
    pending->allowed_flow_count = fc->allowed_count;

    return old_flows;
}

FORT_API NTSTATUS fort_pending_allow_flows(
        PFORT_PENDING pending, const UINT64 *flow_ids, UINT16 count)
{
    const UINT32 size = pending->allowed_flow_count + count;
    if (size > FORT_PENDING_FLOW_COUNT_MAX)
        return STATUS_INSUFFICIENT_RESOURCES;

    FORT_PENDING_FLOWS_CTX fc = {
        .count = count,
        .allowed_size = (UINT16) size,
        .now = (INT64) KeQueryInterruptTime(),
        .flow_ids = flow_ids,
        .allowed_flows =
                fort_mem_alloc(size * sizeof(FORT_PENDING_ALLOWED_FLOW), FORT_PACKET_POOL_TAG),
    };

    if (fc.allowed_flows == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    FORT_PENDING_WALK walk = {
        .done_func = &fort_pending_packet_listed,
        .ctx = &fc,
    };

    PFORT_PENDING_ALLOWED_FLOW free_flows;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);
    {
        free_flows = fort_pending_allow_walk_locked(pending, &walk);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    /* Reauthorize the allowed connections, re-inject their packets */
    fort_pending_packets_complete(walk.packets, /*inject=*/TRUE);

    /* The new array without the room isn't used */
    const NTSTATUS status =
            (free_flows == fc.allowed_flows) ? STATUS_INSUFFICIENT_RESOURCES : STATUS_SUCCESS;

    if (free_flows != NULL) {
        fort_mem_free(free_flows, FORT_PACKET_POOL_TAG);
    }

    return status;
}

FORT_API BOOL fort_pending_flow_take_allowed(PFORT_PENDING pending, UINT64 flow_id)
{
    if (pending->allowed_flow_count == 0)
        return FALSE;

    BOOL allowed;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    allowed = fort_pending_allowed_flow_remove_locked(pending, flow_id);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return allowed;
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

    /* The Rule's decision can't be checked without the connection */
    if (!pkt->app_asked)
        return FALSE;

    const FORT_APP_PATH path = fort_psname_path(pkt->ps_name);

    const FORT_CONF_APP_FIND_RESULT app =
            fort_conf_app_find(&conf_ref->conf, &path, fort_conf_exe_find, conf_ref);

    return !(fort_conf_app_asked(rc->conf_flags, app.data) || app.data.flags.ask);
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

static PFORT_PENDING_PACKET fort_pending_flow_packet_locked(PFORT_PENDING pending, UINT64 flow_id)
{
    PFORT_PENDING_PACKET pkt = pending->packets_head;

    for (; pkt != NULL; pkt = pkt->next) {
        if (pkt->flow_id == flow_id)
            return pkt;
    }

    return NULL;
}

FORT_API BOOL fort_pending_conn_unlogged(PFORT_PENDING pending, PCFORT_CONF_META_CONN conn)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    PFORT_PENDING_PACKET pkt = fort_pending_flow_packet_locked(pending, conn->flow_id);
    if (pkt != NULL) {
        pkt->unlogged = TRUE;
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return pkt != NULL;
}

static BOOL fort_pending_packet_unlogged(void *ctx, PCFORT_PENDING_PACKET pkt)
{
    UNUSED(ctx);

    return pkt->unlogged;
}

FORT_API void fort_pending_block_unlogged(PFORT_PENDING pending)
{
    FORT_PENDING_WALK walk = {
        .done_func = &fort_pending_packet_unlogged,
    };

    fort_pending_complete_walk(pending, &walk, /*reauth=*/FALSE);
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
