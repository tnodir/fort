/* Fort Firewall Callouts */

#include "fortcout.h"

#include "common/fortdef.h"
#include "common/fortioctl.h"
#include "common/fortprov.h"

#include "fortcoutarg.h"
#include "fortdbg.h"
#include "fortdev.h"
#include "fortps.h"
#include "forttrace.h"
#include "fortutl.h"

static struct
{
    FWPS_CALLOUT0 ale_callouts[FORT_STAT_ALE_CALLOUT_IDS_COUNT];
    FWPS_CALLOUT0 packet_callouts[FORT_STAT_PACKET_CALLOUT_IDS_COUNT];
} g_calloutGlobal;

static void fort_callout_classify_block(FWPS_CLASSIFY_OUT0 *classifyOut)
{
    classifyOut->actionType = FWP_ACTION_BLOCK;
    classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
}

static void fort_callout_classify_drop(FWPS_CLASSIFY_OUT0 *classifyOut)
{
    classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

    fort_callout_classify_block(classifyOut);
}

static void fort_callout_classify_permit(
        const FWPS_FILTER0 *filter, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    classifyOut->actionType = FWP_ACTION_PERMIT;
    if ((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
        classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
    }
}

static void fort_callout_classify_continue(FWPS_CLASSIFY_OUT0 *classifyOut)
{
    classifyOut->actionType = FWP_ACTION_CONTINUE;
}

inline static void fort_callout_ale_set_app_flags(
        PFORT_CALLOUT_ALE_EXTRA cx, FORT_APP_DATA app_data)
{
    cx->app_data_found = TRUE;
    cx->app_data = app_data;
}

static void fort_callout_ale_fill_meta_path(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx)
{
    if (cx->is_path_filled)
        return;

    cx->is_path_filled = TRUE;

    PFORT_CONF_META_CONN conn = &cx->conn;

    PFORT_APP_PATH real_path = &conn->real_path;

    real_path->len = (UINT16) (ca->inMetaValues->processPath->size
            - sizeof(WCHAR)); /* chop terminating zero */
    real_path->buffer = (PCWSTR) ca->inMetaValues->processPath->data;

    PFORT_APP_PATH path = &conn->path;
    BOOL inherited = FALSE;

    if (fort_pstree_get_proc_name(&fort_device()->ps_tree, conn->process_id, path, &inherited)) {
        if (!inherited) {
            *real_path = *path;
        }
    } else {
        *path = *real_path;
    }

    conn->inherited = (UCHAR) inherited;
}

static ip_addr_t fort_callout_meta_ip(PCFORT_CALLOUT_ARG ca, UCHAR ipIndex)
{
    ip_addr_t ip;
    if (ca->isIPv6) {
        ip.v6 = *((ip6_addr_t *) ca->inFixedValues->incomingValue[ipIndex].value.byteArray16);
    } else {
        ip.v4 = ca->inFixedValues->incomingValue[ipIndex].value.uint32;
    }
    return ip;
}

static void fort_callout_ale_fill_meta_conn(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx)
{
    if (cx->is_conn_filled)
        return;

    cx->is_conn_filled = TRUE;

    PFORT_CONF_META_CONN conn = &cx->conn;

    conn->ip_proto = ca->inFixedValues->incomingValue[ca->fi->ipProto].value.uint8;
    conn->is_tcp = (conn->ip_proto == IPPROTO_TCP);

    conn->local_port = ca->inFixedValues->incomingValue[ca->fi->localPort].value.uint16;
    conn->remote_port = ca->inFixedValues->incomingValue[ca->fi->remotePort].value.uint16;

    conn->local_ip = fort_callout_meta_ip(ca, ca->fi->localIp);
}

static FORT_APP_DATA fort_callout_ale_conf_app_data(
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref)
{
    if (cx->app_data_found)
        return cx->app_data;

    const FORT_APP_DATA app_data =
            fort_conf_app_find(&conf_ref->conf, &cx->conn.path, fort_conf_exe_find, conf_ref);

    fort_callout_ale_set_app_flags(cx, app_data);

    return app_data;
}

inline static BOOL fort_callout_ale_associate_flow(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, FORT_APP_FLAGS app_flags)
{
    const UINT64 flow_id = ca->inMetaValues->flowHandle;

    fort_callout_ale_fill_meta_conn(ca, cx);

    PFORT_CONF_META_CONN conn = &cx->conn;

    const UCHAR group_index = (UCHAR) app_flags.group_index;

    BOOL log_stat = FALSE;

    const NTSTATUS status =
            fort_flow_associate(&fort_device()->stat, flow_id, conn, group_index, &log_stat);

    if (!NT_SUCCESS(status)) {
        if (status != FORT_STATUS_FLOW_BLOCK) {
            LOG("Classify v4: Flow assoc. error: %x\n", status);
            TRACE(FORT_CALLOUT_FLOW_ASSOC_ERROR, status, 0, 0);
        }

        conn->block_reason = FORT_BLOCK_REASON_REAUTH;
        return TRUE; /* block (Error) */
    }

    if (!log_stat) {
        fort_buffer_proc_new_write(&fort_device()->buffer, conn, &cx->irp, &cx->info);
    }

    return FALSE;
}

inline static BOOL fort_callout_ale_log_app_path_check(
        FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    return app_data.found == 0 && conf_flags.filter_enabled
            && (conf_flags.allow_all_new || conf_flags.log_blocked);
}

inline static void fort_callout_ale_log_app_path(PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    if (cx->ignore || !fort_callout_ale_log_app_path_check(conf_flags, app_data))
        return;

    PFORT_CONF_META_CONN conn = &cx->conn;

    app_data.flags.log_blocked = TRUE;
    app_data.flags.log_conn = TRUE;
    app_data.flags.blocked = (UCHAR) conn->blocked;

    app_data.is_new = TRUE;
    app_data.found = TRUE;
    app_data.alerted = TRUE;

    FORT_APP_ENTRY app_entry = {
        .app_data = app_data,
        .path_len = conn->path.len,
    };

    if (!NT_SUCCESS(fort_conf_ref_exe_add_path(conf_ref, &app_entry, &conn->path)))
        return;

    fort_callout_ale_set_app_flags(cx, app_data);

    fort_buffer_blocked_write(&fort_device()->buffer, conn, &cx->irp, &cx->info);
}

inline static BOOL fort_callout_ale_log_blocked_ip_check_app(
        FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    return (app_data.found == 0 || app_data.flags.log_blocked)
            && (app_data.alerted || !conf_flags.log_alerted_blocked_ip);
}

inline static BOOL fort_callout_ale_log_blocked_ip_check(PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    if (cx->conn.block_reason == FORT_BLOCK_REASON_UNKNOWN)
        return FALSE;

    if (!(conf_flags.ask_to_connect || conf_flags.log_blocked_ip))
        return FALSE;

    fort_callout_ale_fill_meta_path(ca, cx);

    const FORT_APP_DATA app_data = fort_callout_ale_conf_app_data(cx, conf_ref);

    return fort_callout_ale_log_blocked_ip_check_app(conf_flags, app_data);
}

inline static void fort_callout_ale_log_blocked_ip(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx)
{
    fort_callout_ale_fill_meta_conn(ca, cx);

    fort_buffer_blocked_ip_write(&fort_device()->buffer, &cx->conn, &cx->irp, &cx->info);
}

inline static BOOL fort_callout_ale_add_pending(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    if (!fort_pending_add_packet(&fort_device()->pending, ca, cx)) {
        conn->block_reason = FORT_BLOCK_REASON_ASK_LIMIT;
        return TRUE; /* block (error) */
    }

    cx->drop_blocked = TRUE;
    conn->block_reason = FORT_BLOCK_REASON_ASK_PENDING;
    return TRUE; /* drop (pending) */
}

inline static BOOL fort_callout_ale_process_flow(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    if (app_data.found == 0 && conf_flags.ask_to_connect) {
        return fort_callout_ale_add_pending(ca, cx);
    }

    if (!conf_flags.log_stat)
        return FALSE;

    return fort_callout_ale_associate_flow(ca, cx, app_data.flags);
}

inline static BOOL fort_callout_ale_ip_zone_check(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, UINT32 zones_mask, BOOL included)
{
    if (zones_mask == 0)
        return FALSE;

    const BOOL ip_included = fort_conf_zones_ip_included(
            &fort_device()->conf, zones_mask, cx->conn.remote_ip, ca->isIPv6);

    return ip_included == included;
}

static BOOL fort_callout_ale_app_blocked(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    if (fort_conf_app_group_blocked(conf_flags, app_data)) {
        conn->block_reason = FORT_BLOCK_REASON_APP_GROUP_FOUND;
        return TRUE; /* block Group */
    }

    if (app_data.flags.blocked) {
        conn->block_reason = FORT_BLOCK_REASON_PROGRAM;
        return TRUE; /* block Program */
    }

    if (app_data.flags.lan_only && !conn->is_local_net) {
        conn->block_reason = FORT_BLOCK_REASON_LAN_ONLY;
        return TRUE; /* block LAN Only */
    }

    if (fort_callout_ale_ip_zone_check(ca, cx, app_data.reject_zones, /*included=*/TRUE)
            || fort_callout_ale_ip_zone_check(ca, cx, app_data.accept_zones, /*included=*/FALSE)) {
        conn->block_reason = FORT_BLOCK_REASON_ZONE;
        return TRUE; /* block Rejected or Not Accepted Zones */
    }

    return FALSE;
}

inline static BOOL fort_callout_ale_flags_allowed(
        PFORT_CALLOUT_ALE_EXTRA cx, FORT_CONF_FLAGS conf_flags)
{
    /* "Auto-Learn" or "Ask to Connect" */
    if (conf_flags.allow_all_new || conf_flags.ask_to_connect)
        return TRUE;

    /* Block/Allow All */
    if (conf_flags.app_block_all || conf_flags.app_allow_all) {
        return conf_flags.app_allow_all;
    }

    /* Ignore */
    cx->ignore = TRUE;
    cx->conn.block_reason = FORT_BLOCK_REASON_NONE; /* Don't block or allow */
    return TRUE;
}

inline static BOOL fort_callout_ale_is_allowed(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    /* Collect traffic, when Filter Disabled */
    if (!cx->conn.blocked)
        return TRUE;

    if (app_data.found != 0) {
        /* Check app is blocked */
        return !fort_callout_ale_app_blocked(ca, cx, conf_flags, app_data);
    }

    return fort_callout_ale_flags_allowed(cx, conf_flags);
}

inline static void fort_callout_ale_check_app(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    const FORT_APP_DATA app_data = fort_callout_ale_conf_app_data(cx, conf_ref);

    if (fort_callout_ale_is_allowed(ca, cx, conf_flags, app_data)) {

        if (fort_callout_ale_process_flow(ca, cx, conf_flags, app_data))
            return;

        cx->conn.blocked = FALSE; /* allow */
    }

    fort_callout_ale_log_app_path(cx, conf_ref, conf_flags, app_data);
}

inline static BOOL fort_callout_ale_check_filter_lan_flags(
        PFORT_CONF_META_CONN conn, FORT_CONF_FLAGS conf_flags)
{
    if (conf_flags.block_lan_traffic && !conn->is_loopback) {
        return TRUE; /* block LAN */
    }

    if (!conf_flags.filter_local_net) {
        conn->blocked = FALSE;
        return TRUE; /* allow Local Network */
    }

    return FALSE;
}

inline static BOOL fort_callout_ale_check_filter_inet_flags(
        PFORT_CONF_META_CONN conn, FORT_CONF_FLAGS conf_flags)
{
    if (conf_flags.block_inet_traffic) {
        return TRUE; /* block Internet */
    }

    return FALSE;
}

inline static BOOL fort_callout_ale_check_filter_net_flags(
        PFORT_CONF_META_CONN conn, FORT_CONF_FLAGS conf_flags)
{
    if (conn->is_local_net) {
        return fort_callout_ale_check_filter_lan_flags(conn, conf_flags);
    } else {
        return fort_callout_ale_check_filter_inet_flags(conn, conf_flags);
    }
}

inline static BOOL fort_callout_ale_check_filter_flags(
        PFORT_CONF_META_CONN conn, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    if (conf_flags.block_traffic) {
        return TRUE; /* block all */
    }

    conn->is_local_net = !fort_conf_ip_is_inet(&conf_ref->conf,
            (fort_conf_zones_ip_included_func *) &fort_conf_zones_ip_included, &fort_device()->conf,
            conn->remote_ip, conn->isIPv6);

    if (fort_callout_ale_check_filter_net_flags(conn, conf_flags)) {
        return TRUE; /* block net */
    }

    if (!fort_conf_ip_inet_included(&conf_ref->conf,
                (fort_conf_zones_ip_included_func *) &fort_conf_zones_ip_included,
                &fort_device()->conf, conn->remote_ip, conn->isIPv6)) {
        conn->block_reason = FORT_BLOCK_REASON_IP_INET;
        return TRUE; /* block address */
    }

    return FALSE;
}

inline static BOOL fort_callout_ale_check_flags(
        PFORT_CONF_META_CONN conn, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    if (conf_flags.filter_enabled) {
        return fort_callout_ale_check_filter_flags(conn, conf_ref, conf_flags);
    }

    conn->blocked = FALSE;

    if (!(conf_flags.log_stat && conf_flags.log_stat_no_filter))
        return TRUE; /* allow (Filter Disabled) */

    return FALSE;
}

inline static void fort_callout_ale_classify_blocked(PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    /* Log the blocked connection */
    if (fort_callout_ale_log_blocked_ip_check(ca, cx, conf_ref, conf_flags)) {
        fort_callout_ale_log_blocked_ip(ca, cx);
    }

    if (cx->drop_blocked) {
        /* Drop the connection */
        fort_callout_classify_drop(ca->classifyOut);
    } else {
        /* Block the connection */
        fort_callout_classify_block(ca->classifyOut);
    }
}

inline static void fort_callout_ale_classify_action(PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    if (cx->ignore) {
        /* Continue the search */
        fort_callout_classify_continue(ca->classifyOut);
    } else if (!cx->conn.blocked) {
        /* Allow the connection */
        fort_callout_classify_permit(ca->filter, ca->classifyOut);
    } else {
        /* Block/Drop the connection */
        fort_callout_ale_classify_blocked(ca, cx, conf_ref, conf_flags);
    }
}

inline static void fort_callout_ale_check_conf(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    conn->process_id = (UINT32) ca->inMetaValues->processId;

    cx->ignore = FALSE;
    conn->blocked = TRUE;
    conn->block_reason = FORT_BLOCK_REASON_UNKNOWN;

    const FORT_CONF_FLAGS conf_flags = conf_ref->conf.flags;

    if (!fort_callout_ale_check_flags(conn, conf_ref, conf_flags)) {
        fort_callout_ale_fill_meta_path(ca, cx);
        fort_callout_ale_check_app(ca, cx, conf_ref, conf_flags);
    }

    fort_callout_ale_classify_action(ca, cx, conf_ref, conf_flags);
}

inline static void fort_callout_ale_by_conf(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, PFORT_DEVICE_CONF device_conf)
{
    PFORT_CONF_REF conf_ref = fort_conf_ref_take(device_conf);

    if (conf_ref == NULL) {
        if (fort_device_flag(device_conf, FORT_DEVICE_BOOT_FILTER) != 0) {
            fort_callout_classify_block(ca->classifyOut);
        } else {
            fort_callout_classify_continue(ca->classifyOut);
        }
        return;
    }

    cx->irp = NULL;

    fort_callout_ale_check_conf(ca, cx, conf_ref);

    fort_conf_ref_put(device_conf, conf_ref);

    if (cx->irp != NULL) {
        fort_buffer_irp_clear_pending(cx->irp);
        fort_request_complete_info(cx->irp, STATUS_SUCCESS, cx->info);
    }
}

inline static BOOL fort_addr_is_local_multicast(PCFORT_CONF_META_CONN conn)
{
    if (conn->isIPv6) {
        return conn->remote_ip.v2 == 0x2FF;
    }

    return conn->remote_ip.v4 == 0xFFFFFFFF;
}

inline static BOOL fort_callout_ale_is_local_address(PFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, FORT_CONF_FLAGS conf_flags, const UINT32 classify_flags)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    conn->is_loopback = (classify_flags & FWP_CONDITION_FLAG_IS_LOOPBACK) != 0;

    if (conf_flags.filter_locals)
        return FALSE;

    /* Loopback */
    if (!conn->is_loopback || conf_flags.block_traffic)
        return FALSE;

    /* Multicast */
    if (!fort_addr_is_local_multicast(conn) || conf_flags.block_lan_traffic)
        return FALSE;

    return TRUE;
}

static void fort_callout_ale_classify(PFORT_CALLOUT_ARG ca)
{
    FORT_CHECK_STACK(FORT_CALLOUT_ALE_CLASSIFY);

    const UINT32 classify_flags = ca->inFixedValues->incomingValue[ca->fi->flags].value.uint32;

    const BOOL is_reauth = (classify_flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE) != 0;
    if (is_reauth) {
        ca->inbound = (ca->inMetaValues->packetDirection == FWP_DIRECTION_INBOUND);
    }

    FORT_CALLOUT_ALE_EXTRA cx = {
        .conn = {
                .inbound = ca->inbound,
                .isIPv6 = ca->isIPv6,
                .is_reauth = is_reauth,
                .remote_ip = fort_callout_meta_ip(ca, ca->fi->remoteIp),
        },
    };

    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;
    const FORT_CONF_FLAGS conf_flags = device_conf->conf_flags;

    if (fort_callout_ale_is_local_address(ca, &cx, conf_flags, classify_flags)) {
        fort_callout_classify_permit(ca->filter, ca->classifyOut);
    } else {
        fort_callout_ale_by_conf(ca, &cx, device_conf);
    }
}

inline static void fort_callout_ale_classify_v(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        PCFORT_CALLOUT_FIELD_INDEX fi, BOOL inbound, BOOL isIPv6)
{
    FORT_CALLOUT_ARG ca = {
        .fi = fi,
        .inFixedValues = inFixedValues,
        .inMetaValues = inMetaValues,
        .netBufList = layerData,
        .filter = filter,
        .classifyOut = classifyOut,
        .flowContext = flowContext,
        .inbound = inbound,
        .isIPv6 = isIPv6,
    };

    fort_callout_ale_classify(&ca);
}

static void NTAPI fort_callout_connect_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    static const FORT_CALLOUT_FIELD_INDEX fi = {
        .flags = FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL,
    };

    fort_callout_ale_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*inbound=*/FALSE, /*isIPv6=*/FALSE);
}

static void NTAPI fort_callout_connect_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    static const FORT_CALLOUT_FIELD_INDEX fi = {
        .flags = FWPS_FIELD_ALE_AUTH_CONNECT_V6_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_PROTOCOL,
    };

    fort_callout_ale_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*inbound=*/FALSE, /*isIPv6=*/TRUE);
}

static void NTAPI fort_callout_accept_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    static const FORT_CALLOUT_FIELD_INDEX fi = {
        .flags = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL,
    };

    fort_callout_ale_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*inbound=*/TRUE, /*isIPv6=*/FALSE);
}

static void NTAPI fort_callout_accept_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    static const FORT_CALLOUT_FIELD_INDEX fi = {
        .flags = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_PROTOCOL,
    };

    fort_callout_ale_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*inbound=*/TRUE, /*isIPv6=*/TRUE);
}

static NTSTATUS NTAPI fort_callout_notify(
        FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID *filterKey, FWPS_FILTER0 *filter)
{
    UNUSED(notifyType);
    UNUSED(filterKey);
    UNUSED(filter);

    return STATUS_SUCCESS;
}

inline static UINT32 fort_packet_data_size(const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
        const PNET_BUFFER_LIST netBufList, BOOL inbound)
{
    if (netBufList == NULL)
        return 0;

    PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList);
    const UINT32 dataSize = NET_BUFFER_DATA_LENGTH(netBuf);

    const UINT32 headerSize =
            inbound ? inMetaValues->ipHeaderSize + inMetaValues->transportHeaderSize : 0;

    return dataSize + headerSize;
}

static void fort_callout_transport_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        BOOL inbound)
{
    FORT_CHECK_STACK(FORT_CALLOUT_TRANSPORT_CLASSIFY);

    if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0
            || classifyOut->actionType == FWP_ACTION_BLOCK)
        return; /* Can't act on the packet */

    const PNET_BUFFER_LIST netBufList = layerData;

    FORT_CALLOUT_ARG ca = {
        .inFixedValues = inFixedValues,
        .inMetaValues = inMetaValues,
        .netBufList = netBufList,
        .filter = filter,
        .classifyOut = classifyOut,
        .flowContext = flowContext,
        .dataSize = fort_packet_data_size(inMetaValues, netBufList, inbound),
        .inbound = inbound,
    };

    if (fort_shaper_packet_process(&fort_device()->shaper, &ca)) {
        fort_callout_classify_drop(classifyOut); /* drop */
    } else {
        fort_flow_classify(&fort_device()->stat, flowContext, ca.dataSize, inbound);

        fort_callout_classify_permit(filter, classifyOut); /* permit */
    }
}

static void NTAPI fort_callout_transport_classify_in(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_transport_classify(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, /*inbound=*/TRUE);
}

static void NTAPI fort_callout_transport_classify_out(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_transport_classify(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, /*inbound=*/FALSE);
}

static void NTAPI fort_callout_flow_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);

    FORT_CHECK_STACK(FORT_CALLOUT_TRANSPORT_DELETE);

    fort_shaper_drop_flow_packets(&fort_device()->shaper, flowContext);

    fort_flow_delete(&fort_device()->stat, flowContext);
}

static void NTAPI fort_callout_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);
    UNUSED(flowContext);
}

static void fort_callout_init_callout(FWPS_CALLOUT0 *cout, GUID calloutKey,
        FWPS_CALLOUT_CLASSIFY_FN0 classifyFn, FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN0 flowDeleteFn,
        UINT32 flags)
{
    cout->calloutKey = calloutKey;
    cout->flags = flags;
    cout->classifyFn = classifyFn;
    cout->notifyFn = &fort_callout_notify;
    cout->flowDeleteFn = flowDeleteFn;
}

inline static void fort_callout_init_ale_callout(
        FWPS_CALLOUT0 *cout, GUID calloutKey, FWPS_CALLOUT_CLASSIFY_FN0 classifyFn)
{
    fort_callout_init_callout(cout, calloutKey, classifyFn,
            /*flowDeleteFn=*/NULL, /*flags=*/0);
}

static void fort_callout_init_ale_callouts(void)
{
    FWPS_CALLOUT0 *cout = g_calloutGlobal.ale_callouts;

    /* IPv4 connect callout */
    fort_callout_init_ale_callout(cout++, FORT_GUID_CALLOUT_CONNECT_V4, &fort_callout_connect_v4);
    /* IPv6 connect callout */
    fort_callout_init_ale_callout(cout++, FORT_GUID_CALLOUT_CONNECT_V6, &fort_callout_connect_v6);
    /* IPv4 accept callout */
    fort_callout_init_ale_callout(cout++, FORT_GUID_CALLOUT_ACCEPT_V4, &fort_callout_accept_v4);
    /* IPv6 accept callout */
    fort_callout_init_ale_callout(cout++, FORT_GUID_CALLOUT_ACCEPT_V6, &fort_callout_accept_v6);
}

inline static void fort_callout_init_packet_callout(FWPS_CALLOUT0 *cout, GUID calloutKey,
        FWPS_CALLOUT_CLASSIFY_FN0 classifyFn, FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN0 flowDeleteFn)
{
    fort_callout_init_callout(
            cout, calloutKey, classifyFn, flowDeleteFn, FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW);
}

static void fort_callout_init_packet_callouts(void)
{
    FWPS_CALLOUT0 *cout = g_calloutGlobal.packet_callouts;

    /* IPv4 inbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_V4,
            &fort_callout_transport_classify_in, &fort_callout_flow_delete);
    /* IPv6 inbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_V6,
            &fort_callout_transport_classify_in, &fort_callout_flow_delete);
    /* IPv4 outbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_OUT_TRANSPORT_V4,
            &fort_callout_transport_classify_out, &fort_callout_delete);
    /* IPv6 outbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_OUT_TRANSPORT_V6,
            &fort_callout_transport_classify_out, &fort_callout_delete);
}

static void fort_callout_init(void)
{
    RtlZeroMemory(&g_calloutGlobal, sizeof(g_calloutGlobal));

    fort_callout_init_ale_callouts();
    fort_callout_init_packet_callouts();
}

static NTSTATUS fort_callout_register(
        PDEVICE_OBJECT device, const FWPS_CALLOUT0 *callouts, const PUINT32 calloutIds, int count)
{
    for (int i = 0; i < count; ++i) {
        const NTSTATUS status = FwpsCalloutRegister0(device, &callouts[i], &calloutIds[i]);
        if (!NT_SUCCESS(status)) {
            LOG("Callout Register: Error: %x\n", status);
            TRACE(FORT_CALLOUT_REGISTER_ERROR, status, i, 0);
            return status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS fort_callout_install_ale(PDEVICE_OBJECT device, PFORT_STAT stat)
{
    const PUINT32 calloutIds = &stat->callout_ids[FORT_STAT_ALE_CALLOUT_IDS_INDEX];

    return fort_callout_register(
            device, g_calloutGlobal.ale_callouts, calloutIds, FORT_STAT_ALE_CALLOUT_IDS_COUNT);
}

static NTSTATUS fort_callout_install_packet(PDEVICE_OBJECT device, PFORT_STAT stat)
{
    const PUINT32 calloutIds = &stat->callout_ids[FORT_STAT_PACKET_CALLOUT_IDS_INDEX];

    return fort_callout_register(device, g_calloutGlobal.packet_callouts, calloutIds,
            FORT_STAT_PACKET_CALLOUT_IDS_COUNT);
}

FORT_API NTSTATUS fort_callout_install(PDEVICE_OBJECT device)
{
    FORT_CHECK_STACK(FORT_CALLOUT_INSTALL);

    PFORT_STAT stat = &fort_device()->stat;

    fort_callout_init();

    NTSTATUS status;
    if (!NT_SUCCESS(status = fort_callout_install_ale(device, stat))
            || !NT_SUCCESS(status = fort_callout_install_packet(device, stat))) {
        return status;
    }

    return STATUS_SUCCESS;
}

FORT_API void fort_callout_remove(void)
{
    FORT_CHECK_STACK(FORT_CALLOUT_REMOVE);

    PFORT_STAT stat = &fort_device()->stat;

    const PUINT32 calloutIds = stat->callout_ids;

    for (int i = 0; i < FORT_STAT_CALLOUT_IDS_COUNT; ++i) {
        PUINT32 calloutId = &calloutIds[i];
        FwpsCalloutUnregisterById0(*calloutId);
        *calloutId = 0;
    }
}

inline static NTSTATUS fort_callout_force_reauth_prov_flow_filters(HANDLE engine,
        const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags, BOOL force)
{
    const BOOL conf_changed = (old_conf_flags.log_stat != conf_flags.log_stat);

    if (!force && !conf_changed)
        return STATUS_SUCCESS;

    fort_prov_flow_unregister(engine);

    if (!conf_flags.log_stat)
        return STATUS_SUCCESS;

    return fort_prov_flow_register(engine);
}

inline static NTSTATUS fort_callout_force_reauth_prov_recreate(HANDLE engine,
        const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags,
        BOOL *prov_recreated)
{
    const BOOL conf_changed = (old_conf_flags.boot_filter != conf_flags.boot_filter
            || old_conf_flags.filter_locals != conf_flags.filter_locals);

    if (!conf_changed)
        return STATUS_SUCCESS;

    const FORT_PROV_BOOT_CONF boot_conf = {
        .boot_filter = conf_flags.boot_filter,
        .filter_locals = conf_flags.filter_locals,
    };

    fort_prov_unregister(engine);

    const NTSTATUS status = fort_prov_register(engine, boot_conf);
    if (status == 0) {
        *prov_recreated = TRUE;
    }

    return status;
}

inline static NTSTATUS fort_callout_force_reauth_prov_filters(
        HANDLE engine, const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags)
{
    NTSTATUS status;

    /* Check provider filters */
    BOOL prov_recreated = FALSE;
    status = fort_callout_force_reauth_prov_recreate(
            engine, old_conf_flags, conf_flags, &prov_recreated);
    if (status != 0)
        return status;

    /* Check flow filter */
    status = fort_callout_force_reauth_prov_flow_filters(engine, old_conf_flags, conf_flags,
            /*force=*/prov_recreated);
    if (status != 0)
        return status;

    /* Force reauth filter */
    fort_prov_reauth(engine);

    return STATUS_SUCCESS;
}

inline static NTSTATUS fort_callout_force_reauth_prov(
        const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags)
{
    NTSTATUS status;

    HANDLE engine;
    status = fort_prov_trans_open(&engine);
    if (!NT_SUCCESS(status))
        return status;

    status = fort_callout_force_reauth_prov_filters(engine, old_conf_flags, conf_flags);

    return fort_prov_trans_close(engine, status);
}

FORT_API NTSTATUS fort_callout_force_reauth(const FORT_CONF_FLAGS old_conf_flags)
{
    FORT_CHECK_STACK(FORT_CALLOUT_FORCE_REAUTH);

    NTSTATUS status;

    const FORT_CONF_FLAGS conf_flags = fort_device()->conf.conf_flags;

    /* Handle log_stat */
    fort_stat_log_update(&fort_device()->stat, conf_flags.log_stat);

    /* Run the log_timer */
    fort_timer_set_running(&fort_device()->log_timer, /*run=*/conf_flags.log_stat);

    /* Reauth provider filters */
    status = fort_callout_force_reauth_prov(old_conf_flags, conf_flags);

    if (!NT_SUCCESS(status)) {
        LOG("Callout Reauth: Error: %x\n", status);
        TRACE(FORT_CALLOUT_CALLOUT_REAUTH_ERROR, status, 0, 0);
    }

    return status;
}

inline static void fort_callout_update_system_time(
        PFORT_STAT stat, PFORT_BUFFER buf, PIRP *irp, ULONG_PTR *info)
{
    LARGE_INTEGER system_time;
    KeQuerySystemTime(&system_time);

    if (stat->system_time.QuadPart == system_time.QuadPart)
        return;

    stat->system_time = system_time;

    PCHAR out;
    if (NT_SUCCESS(fort_buffer_prepare(buf, FORT_LOG_TIME_SIZE, &out, irp, info))) {
        const INT64 unix_time = fort_system_to_unix_time(system_time.QuadPart);

        const UCHAR old_stat_flags =
                fort_stat_flags_set(stat, FORT_STAT_SYSTEM_TIME_CHANGED, FALSE);
        const BOOL system_time_changed = (old_stat_flags & FORT_STAT_SYSTEM_TIME_CHANGED) != 0;

        fort_log_time_write(out, system_time_changed, unix_time);
    }
}

inline static void fort_callout_flush_stat_traf(
        PFORT_STAT stat, PFORT_BUFFER buf, PIRP *irp, ULONG_PTR *info)
{
    while (stat->proc_active_count != 0) {
        const UINT16 proc_count = (stat->proc_active_count < FORT_LOG_STAT_BUFFER_PROC_COUNT)
                ? stat->proc_active_count
                : FORT_LOG_STAT_BUFFER_PROC_COUNT;
        const UINT32 len = FORT_LOG_STAT_SIZE(proc_count);
        PCHAR out;

        const NTSTATUS status = fort_buffer_prepare(buf, len, &out, irp, info);
        if (!NT_SUCCESS(status)) {
            LOG("Callout Timer: Error: %x\n", status);
            TRACE(FORT_CALLOUT_CALLOUT_TIMER_ERROR, status, 0, 0);
            break;
        }

        fort_log_stat_traf_header_write(out, proc_count);
        out += FORT_LOG_STAT_HEADER_SIZE;

        fort_stat_traf_flush(stat, proc_count, out);
    }
}

FORT_API void fort_callout_timer(void)
{
    FORT_CHECK_STACK(FORT_CALLOUT_TIMER);

    PFORT_BUFFER buf = &fort_device()->buffer;
    PFORT_STAT stat = &fort_device()->stat;

    PIRP irp = NULL;
    ULONG_PTR info;

    /* Lock buffer */
    KLOCK_QUEUE_HANDLE buf_lock_queue;
    fort_buffer_dpc_begin(buf, &buf_lock_queue);

    /* Lock stat */
    KLOCK_QUEUE_HANDLE stat_lock_queue;
    fort_stat_dpc_begin(stat, &stat_lock_queue);

    /* Get current Unix time */
    fort_callout_update_system_time(stat, buf, &irp, &info);

    /* Flush traffic statistics */
    fort_callout_flush_stat_traf(stat, buf, &irp, &info);

    /* Unlock stat */
    fort_stat_dpc_end(&stat_lock_queue);

    /* Flush pending buffer */
    if (irp == NULL) {
        fort_buffer_flush_pending(buf, &irp, &info);
    }

    /* Unlock buffer */
    fort_buffer_dpc_end(&buf_lock_queue);

    if (irp != NULL) {
        fort_buffer_irp_clear_pending(irp);
        fort_request_complete_info(irp, STATUS_SUCCESS, info);
    }
}
