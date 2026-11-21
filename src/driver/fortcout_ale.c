/* Fort Firewall Callouts: ALE */

#include "fortcout_ale.h"

#include "common/fortconf_conn.h"
#include "common/fortdef.h"

#include "fortcnf_conf.h"
#include "fortcnf_group.h"
#include "fortcnf_host.h"
#include "fortcnf_rule.h"
#include "fortcnf_zone.h"
#include "fortcout.h"
#include "fortcoutarg.h"
#include "fortdbg.h"
#include "fortdev.h"
#include "fortps.h"
#include "forttrace.h"
#include "fortutl.h"

inline static void fort_callout_ale_set_app_flags(
        PFORT_CONF_META_CONN conn, const FORT_APP_DATA app_data)
{
    conn->app_data_filled = TRUE;
    conn->app.data = app_data;
}

inline static void fort_callout_ale_fill_meta_path_real(PFORT_CONF_META_CONN conn,
        const FWP_BYTE_BLOB processPath, const FORT_APP_PATH_DRIVE ps_drive)
{
    PFORT_PATH_BUFFER pb = &conn->path_buf;
    PFORT_APP_PATH path = &pb->path;

    if (processPath.size > FORT_PATH_BUFFER_DATA_MIN_SIZE) {
        if (!fort_path_buffer_alloc(pb, processPath.size))
            return;

        path->buffer = pb->buffer;
    } else {
        path->buffer = pb->data;
    }

    path->len = (UINT16) (processPath.size - sizeof(WCHAR)); /* chop terminating zero */

    RtlCopyMemory((PVOID) path->buffer, processPath.data, processPath.size);

    fort_path_drive_adjust(path, ps_drive);

    conn->real_path = *path;
}

inline static FWP_BYTE_BLOB fort_callout_meta_process_path(
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues)
{
    if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_PROCESS_PATH)
            && inMetaValues->processPath->size >= sizeof(WCHAR)) {

        return *inMetaValues->processPath;
    }

    /* The empty path with terminating zero */
    const FWP_BYTE_BLOB emptyPath = {
        .size = sizeof(WCHAR),
        .data = (UINT8 *) L"",
    };

    return emptyPath;
}

static void fort_callout_ale_fill_meta_path(PCFORT_CALLOUT_ARG ca, PFORT_CONF_META_CONN conn)
{
    const FWP_BYTE_BLOB processPath = fort_callout_meta_process_path(ca->inMetaValues);

    PFORT_APP_PATH path = &conn->path;

    path->len = (UINT16) (processPath.size - sizeof(WCHAR)); /* chop terminating zero */
    path->buffer = (PCWSTR) processPath.data;

    PFORT_APP_PATH real_path = &conn->real_path;
    *real_path = *path;

    FORT_PS_INFO ps_info = { 0 };

    conn->ps_name =
            fort_pstree_get_proc_name(&fort_device()->ps_tree, conn->process_id, path, &ps_info);

    conn->parent_process_id = ps_info.parent_process_id;
    conn->create_time = ps_info.create_time;

    if (conn->ps_name != NULL) {

        const BOOL inherited = (ps_info.opt.flags & FORT_PSNODE_NAME_INHERITED) != 0;
        if (!inherited) {
            *real_path = *path;
            return;
        }

        conn->inherited = TRUE;
    }

    fort_callout_ale_fill_meta_path_real(conn, processPath, ps_info.opt.path_drive);

    if (!conn->inherited) {
        *path = *real_path;
    }
}

inline static void fort_callout_fill_meta_port(PCFORT_CALLOUT_ARG ca, UCHAR portIndex, UINT16 *port)
{
    const FWP_VALUE0 value = ca->inFixedValues->incomingValue[portIndex].value;

    *port = value.uint16;
}

static void fort_callout_fill_meta_ip(PCFORT_CALLOUT_ARG ca, UCHAR ipIndex, ip_addr_t *ip)
{
    const FWP_VALUE0 value = ca->inFixedValues->incomingValue[ipIndex].value;

    if (ca->isIPv6) {
        RtlCopyMemory(ip->v6.data, value.byteArray16, sizeof(ip6_addr_t));
    } else {
        ip->v4 = value.uint32;
    }
}

inline static void fort_callout_ale_fill_meta_conn_proc(
        PCFORT_CALLOUT_ARG ca, PFORT_CONF_META_CONN conn)
{
    const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues = ca->inMetaValues;

    conn->process_id = FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_PROCESS_ID)
            ? (UINT32) inMetaValues->processId
            : 0;

    const UINT32 client_process_id = fort_device()->client_process_id;

    conn->is_fort_service = (client_process_id != 0 && conn->process_id == client_process_id);
}

static void fort_callout_ale_fill_meta_conn(PCFORT_CALLOUT_ARG ca, PFORT_CONF_META_CONN conn)
{
    conn->flow_id = ca->inMetaValues->flowHandle;

    conn->profile_id = ca->inFixedValues->incomingValue[ca->fi->profileId].value.uint8;

    conn->ip_proto = ca->inFixedValues->incomingValue[ca->fi->ipProto].value.uint8;

    fort_callout_fill_meta_port(ca, ca->fi->localPort, &conn->local_port);
    fort_callout_fill_meta_ip(ca, ca->fi->localIp, &conn->local_ip);
}

static FORT_APP_DATA fort_callout_ale_conf_app_data(
        PCFORT_CALLOUT_ARG ca, PFORT_CONF_META_CONN conn, PFORT_CONF_REF conf_ref)
{
    if (conn->app_data_filled) {
        return conn->app.data;
    }

    fort_callout_ale_fill_meta_path(ca, conn);

    conn->app = fort_conf_app_find(&conf_ref->conf, &conn->path, fort_conf_exe_find, conf_ref);
    conn->app_data_filled = TRUE;

    return conn->app.data;
}

inline static BOOL fort_callout_ale_associate_flow_log(
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_META_CONN conn)
{
    const NTSTATUS status = fort_buffer_conn_write(
            &fort_device()->buffer, conn, &cx->irp_info, FORT_BUFFER_CONN_WRITE_PROC_NEW);

    /* Log the process again by its next flow */
    if (!NT_SUCCESS(status)) {
        fort_flow_proc_unlog(&fort_device()->stat, conn->process_id);
        return FALSE;
    }

    fort_flow_proc_written(&fort_device()->stat, conn);
    return TRUE;
}

/* Is the process's PROC_NEW in the log before the connection's CONN? */
inline static BOOL fort_callout_ale_associate_flow_proc_logged(PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_META_CONN conn, const FORT_FLOW_PROC_LOG_TYPE proc_log)
{
    switch (proc_log) {
    case FORT_FLOW_PROC_LOG_NEW:
        return fort_callout_ale_associate_flow_log(cx, conn);
    case FORT_FLOW_PROC_LOG_WRITTEN:
        return TRUE;
    default:
        return FALSE; /* another connection logs it */
    }
}

inline static BOOL fort_callout_ale_associate_flow(
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_META_CONN conn)
{
    FORT_FLOW_ASSOC assoc = { 0 };

    const NTSTATUS status = fort_flow_associate(&fort_device()->stat, conn, &assoc);

    if (status == FORT_STATUS_FLOW_SKIP)
        return FALSE; /* the stat is closed or not yet enabled */

    if (!NT_SUCCESS(status)) {
        if (status != FORT_STATUS_FLOW_BLOCK) {
            LOG("Classify v4: Flow assoc. error: %x\n", status);
            TRACE(FORT_CALLOUT_FLOW_ASSOC_ERROR, status, 0, 0);
        }

        conn->reason = FORT_CONN_REASON_REAUTH;
        return TRUE; /* block (Error) */
    }

    /* The flow's CONN is logged once, not on each reauthorization */
    conn->act.conn_nolog |= (UCHAR) assoc.conn_logged;
    conn->flow_logging = (UINT16) !assoc.conn_logged;

    conn->proc_logged =
            (UINT16) fort_callout_ale_associate_flow_proc_logged(cx, conn, assoc.proc_log);

    return FALSE;
}

inline static BOOL fort_callout_ale_log_app_path_check(
        FORT_CONF_FLAGS conf_flags, const FORT_APP_DATA app_data)
{
    return app_data.flags.found == 0 && conf_flags.filter_enabled
            && (conf_flags.allow_all_new || conf_flags.log_app);
}

inline static void fort_callout_ale_log_app_path(PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, const FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    if (conn->ignore || !fort_callout_ale_log_app_path_check(conf_flags, app_data))
        return;

    /* Fort Firewall's Service is logged allowed, without an alert */
    const BOOL dns_forwarder_allowed = (conn->reason == FORT_CONN_REASON_DNS_FORWARDER);

    app_data.flags.log_stat = TRUE;
    app_data.flags.log_allowed_conn = TRUE;
    app_data.flags.log_blocked_conn = TRUE;
    app_data.flags.blocked =
            !dns_forwarder_allowed && !(conf_flags.allow_all_new || conf_flags.app_allow_all);

    app_data.flags.is_new = TRUE;
    app_data.flags.found = TRUE;
    app_data.flags.alerted = !dns_forwarder_allowed;

    const FORT_APP_ENTRY app_entry = {
        .app_data = app_data,
        .path_len = conn->path.len,
    };

    if (!NT_SUCCESS(fort_conf_ref_exe_add_path(conf_ref, &app_entry, &conn->path)))
        return;

    fort_callout_ale_set_app_flags(conn, app_data);

    fort_buffer_conn_write(&fort_device()->buffer, conn, &cx->irp_info, FORT_BUFFER_CONN_WRITE_APP);
}

inline static BOOL fort_callout_ale_log_conn_check_app(
        PFORT_CONF_META_CONN conn, const FORT_APP_DATA app_data, const FORT_CONF_FLAGS conf_flags)
{
    const BOOL log_conn = app_data.flags.found == 0
            || (conn->act.blocked ? app_data.flags.log_blocked_conn
                                  : app_data.flags.log_allowed_conn);

    if (!log_conn)
        return FALSE;

    conn->act.conn_alert |= app_data.flags.alerted;

    const BOOL log_alert = (conn->act.conn_alert || !conf_flags.log_alerted_conn);

    return log_alert && !conn->act.conn_nolog;
}

inline static BOOL fort_callout_ale_log_conn_check(PCFORT_CALLOUT_ARG ca, PFORT_CONF_META_CONN conn,
        PFORT_CONF_REF conf_ref, const FORT_CONF_FLAGS conf_flags)
{
    if (conn->ignore || conn->reason == FORT_CONN_REASON_UNKNOWN)
        return FALSE;

    /* The pending connection is shown to the user, the decided one is logged by the flags */
    if (conn->reason == FORT_CONN_REASON_ASK_PENDING)
        return TRUE;

    /* Conf */
    {
        const BOOL log_conn =
                (conn->act.blocked ? conf_flags.log_blocked_conn : conf_flags.log_allowed_conn);

        if (!log_conn)
            return FALSE;
    }

    /* App */
    {
        const FORT_APP_DATA app_data = fort_callout_ale_conf_app_data(ca, conn, conf_ref);

        return fort_callout_ale_log_conn_check_app(conn, app_data, conf_flags);
    }
}

inline static void fort_callout_ale_ask_pended(PFORT_CONF_META_CONN conn)
{
    /* Drop the connection's repeated packet or the App's one over its limit silently */
    conn->act.drop_blocked = TRUE;
    conn->act.conn_nolog = TRUE;
}

inline static void fort_callout_ale_add_pending(PCFORT_CALLOUT_ARG ca, PFORT_CONF_META_CONN conn)
{
    const NTSTATUS status = fort_pending_add_packet(&fort_device()->pending, ca, conn);

    if (status == STATUS_ALREADY_REGISTERED) {
        fort_callout_ale_ask_pended(conn);
        return;
    }

    if (!NT_SUCCESS(status)) {
        conn->reason = FORT_CONN_REASON_ASK_LIMIT;
        return; /* block (Limit | Timeout) */
    }

    conn->act.drop_blocked = TRUE;
    conn->reason = FORT_CONN_REASON_ASK_PENDING; /* drop (Pending) */
}

inline static BOOL fort_callout_ale_process_flow(
        PFORT_CALLOUT_ALE_EXTRA cx, const FORT_CONF_FLAGS conf_flags)
{
    if (!conf_flags.log_stat)
        return FALSE;

    return fort_callout_ale_associate_flow(cx, &cx->conn);
}

/* Allowed by the user: till its first reauthorization, then by its associated flow */
inline static BOOL fort_callout_ale_ask_allowed(PCFORT_CONF_META_CONN conn)
{
    return fort_pending_flow_take_allowed(&fort_device()->pending, conn->flow_id)
            || fort_flow_ask_allowed(&fort_device()->stat, conn->flow_id);
}

inline static void fort_callout_ale_ask_decided(PFORT_CALLOUT_ALE_EXTRA cx)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    if (!fort_callout_ale_ask_allowed(conn)) {
        conn->reason = FORT_CONN_REASON_ASK_BLOCKED;
        return; /* block (by the user or the timeout) */
    }

    conn->reason = FORT_CONN_REASON_ASK_ALLOWED;

    /* The associated flow keeps the user's decision */
    conn->act.blocked = (UCHAR) fort_callout_ale_associate_flow(cx, conn); /* allow once */
}

inline static BOOL fort_callout_ale_ask_decided_check(
        PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn)
{
    /* The reauthorization of the completed pending connection or its re-injected packet */
    return conn->is_reauth || fort_packet_injected_by_self(ca);
}

inline static void fort_callout_ale_ask_proc_allowed(
        PFORT_CALLOUT_ALE_EXTRA cx, const FORT_CONF_FLAGS conf_flags)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    conn->reason = FORT_CONN_REASON_ASK_PROC_ALLOWED;

    conn->act.blocked = (UCHAR) fort_callout_ale_process_flow(cx, conf_flags); /* block (Error) */
}

inline static void fort_callout_ale_ask(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, const FORT_CONF_FLAGS conf_flags)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    /* The process is allowed by the user */
    if (fort_pstree_proc_allowed(&fort_device()->ps_tree, conn->process_id)) {
        fort_callout_ale_ask_proc_allowed(cx, conf_flags);
        return;
    }

    /* Decided by the user */
    if (fort_callout_ale_ask_decided_check(ca, conn)) {
        fort_callout_ale_ask_decided(cx);
        return;
    }

    if (fort_pending_conn_pended(&fort_device()->pending, conn)) {
        fort_callout_ale_ask_pended(conn);
        return;
    }

    fort_callout_ale_add_pending(ca, conn);
}

inline static void fort_callout_ale_check_app(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, PCFORT_CONF_CONN_FILTER filter)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    const FORT_CONF_FLAGS conf_flags = filter->conf_flags;

    const FORT_APP_DATA app_data = fort_callout_ale_conf_app_data(ca, conn, conf_ref);

    if (fort_conf_conn_app_allowed(filter, conn, app_data)) {

        if (fort_callout_ale_process_flow(cx, conf_flags)) {
            conn->act.blocked = TRUE; /* block (Error) */
            return;
        }
    } else if (conn->ask_to_connect) {
        fort_callout_ale_ask(ca, cx, conf_flags);
    }

    fort_callout_ale_log_app_path(cx, conf_ref, conf_flags, app_data);
}

static BOOL fort_callout_ale_host_ip_included(
        void *ctx, PCFORT_CONF_META_CONN conn, PCFORT_CONF_RULE_FILTER_HOST host)
{
    return fort_host_ips_filter_included(ctx, conn, host);
}

static BOOL fort_callout_ale_zones_ip_included(
        void *ctx, PCFORT_CONF_META_CONN conn, UCHAR *zone_id, UINT32 zones_mask)
{
    return fort_devconf_zones_ip_included(ctx, conn, zone_id, zones_mask);
}

static BOOL fort_callout_ale_zones_conn_filtered(
        void *ctx, PCFORT_CONF_META_CONN conn, PFORT_CONF_ZONES_CONN_FILTERED_OPT opt)
{
    return fort_devconf_zones_conn_filtered(ctx, conn, opt);
}

static BOOL fort_callout_ale_app_rule_conn_filtered(
        void *ctx, PFORT_CONF_META_CONN conn, PCFORT_CONF_RULE rule)
{
    return fort_devconf_app_rule_conn_filtered(ctx, conn, rule);
}

static UINT16 fort_callout_ale_rules_glob_conn_filtered(
        void *ctx, PFORT_CONF_META_CONN conn, BOOL is_post)
{
    return fort_devconf_rules_glob_conn_filtered(ctx, conn, is_post);
}

static BOOL fort_callout_ale_groups_mask_blocked(void *ctx, UINT32 groups_mask)
{
    return fort_devconf_groups_mask_blocked(ctx, groups_mask);
}

static UINT16 fort_callout_ale_groups_rules_conn_filtered(
        void *ctx, PFORT_CONF_META_CONN conn, UINT32 groups_mask)
{
    return fort_devconf_groups_rules_conn_filtered(ctx, conn, groups_mask);
}

static const FORT_CONF_CONN_FILTER_FUNCS fort_callout_ale_filter_funcs = {
    .zones_ip_included = &fort_callout_ale_zones_ip_included,
    .zones_conn_filtered = &fort_callout_ale_zones_conn_filtered,
    .app_rule_conn_filtered = &fort_callout_ale_app_rule_conn_filtered,
    .rules_glob_conn_filtered = &fort_callout_ale_rules_glob_conn_filtered,
    .groups_mask_blocked = &fort_callout_ale_groups_mask_blocked,
    .groups_rules_conn_filtered = &fort_callout_ale_groups_rules_conn_filtered,
};

inline static void fort_callout_ale_classify_block(
        PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn)
{
    if (conn->act.drop_blocked) {
        fort_callout_classify_drop(ca->classifyOut);
    } else {
        fort_callout_classify_block(ca->classifyOut);
    }
}

inline static void fort_callout_ale_classify_permit(
        PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn)
{
    if (conn->act.permit_hard) {
        fort_callout_classify_permit_hard(ca->classifyOut);
    } else {
        fort_callout_classify_permit(ca->filter, ca->classifyOut);
    }
}

inline static void fort_callout_ale_classify_action(
        PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn)
{
    if (conn->ignore) {
        /* Continue the search */
        fort_callout_classify_continue(ca->classifyOut);
    } else if (conn->act.blocked) {
        /* Block the connection */
        fort_callout_ale_classify_block(ca, conn);
    } else {
        /* Allow the connection */
        fort_callout_ale_classify_permit(ca, conn);
    }
}

inline static void fort_callout_ale_classify_boot_action(
        PCFORT_CALLOUT_ARG ca, PFORT_DEVICE_CONF device_conf)
{
    FWPS_CLASSIFY_OUT0 *classifyOut = ca->classifyOut;

    const BOOL isBootFilter = fort_device_flag(device_conf, FORT_DEVICE_BOOT_FILTER) != 0;
    if (isBootFilter) {
        /* Block the connection */
        fort_callout_classify_block(classifyOut);
    } else {
        /* Continue the search */
        fort_callout_classify_continue(classifyOut);
    }
}

inline static void fort_callout_ale_log_conn_pending_unlogged(PCFORT_CONF_META_CONN conn)
{
    /* The not logged pending connection isn't shown to the user: block it by the worker */
    if (fort_pending_conn_unlogged(&fort_device()->pending, conn)) {
        fort_worker_queue(&fort_device()->worker, FORT_WORKER_PENDING);
    }
}

inline static void fort_callout_ale_log_conn(PFORT_CALLOUT_ALE_EXTRA cx)
{
    PCFORT_CONF_META_CONN conn = &cx->conn;

    const NTSTATUS status = fort_buffer_conn_write(
            &fort_device()->buffer, conn, &cx->irp_info, FORT_BUFFER_CONN_WRITE_CONN);

    if (NT_SUCCESS(status))
        return;

    if (conn->flow_logging) {
        /* Log the flow's connection by its next reauthorization */
        fort_flow_conn_unlog(&fort_device()->stat, conn->flow_id);
    } else if (conn->reason == FORT_CONN_REASON_ASK_PENDING) {
        fort_callout_ale_log_conn_pending_unlogged(conn);
    }
}

inline static void fort_callout_ale_reauth_blocked(PCFORT_CONF_META_CONN conn)
{
    if (conn->is_reauth && conn->act.blocked) {
        fort_flow_reauth_blocked(&fort_device()->stat, conn->flow_id);
    }
}

inline static void fort_callout_ale_check_conf(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, const FORT_CONF_FLAGS conf_flags)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    fort_callout_ale_fill_meta_conn_proc(ca, conn);
    fort_callout_ale_fill_meta_conn(ca, conn);

    PFORT_HOST_IPS host_ips = &fort_device()->host_ips;

    conn->host_zones_mask = fort_host_ips_zones_mask(host_ips, conn);
    conn->host_ip_included = &fort_callout_ale_host_ip_included;
    conn->host_ip_ctx = host_ips;

    const FORT_CONF_CONN_FILTER filter = {
        .conf_flags = conf_flags,
        .conf = &conf_ref->conf,
        .funcs = &fort_callout_ale_filter_funcs,
        .ctx = &fort_device()->conf,
    };

    if (!fort_conf_conn_flags_filtered(&filter, conn)) {
        fort_callout_ale_check_app(ca, cx, conf_ref, &filter);
    }

    fort_callout_ale_reauth_blocked(conn);

    /* Log the connection */
    if (fort_callout_ale_log_conn_check(ca, conn, conf_ref, conf_flags)) {
        fort_callout_ale_log_conn(cx);
    }

    fort_callout_ale_classify_action(ca, conn);

    /* Free the allocated path */
    fort_path_buffer_free(&conn->path_buf);

    /* Release the process's name */
    fort_psname_put(&fort_device()->ps_names, conn->ps_name);
}

inline static void fort_callout_ale_by_conf(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_DEVICE_CONF device_conf, const FORT_CONF_FLAGS conf_flags)
{
    const BOOL ps_enumerated = fort_device_flag(device_conf, FORT_DEVICE_PS_ENUMERATED) != 0;

    PFORT_CONF_REF conf_ref = ps_enumerated ? fort_conf_ref_take(device_conf) : NULL;

    if (conf_ref == NULL) {
        fort_callout_ale_classify_boot_action(ca, device_conf);
        return;
    }

    PFORT_IRP_INFO irp_info = &cx->irp_info;
    irp_info->irp = NULL;

    fort_callout_ale_check_conf(ca, cx, conf_ref, conf_flags);

    fort_conf_ref_put(device_conf, conf_ref);

    if (irp_info->irp != NULL) {
        fort_buffer_irp_clear_pending(irp_info);
        fort_request_complete_info(irp_info, STATUS_SUCCESS);
    }
}

inline static BOOL fort_callout_ale_is_local_address(
        PFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, const FORT_CONF_FLAGS conf_flags)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    fort_callout_fill_meta_port(ca, ca->fi->remotePort, &conn->remote_port);
    fort_callout_fill_meta_ip(ca, ca->fi->remoteIp, &conn->remote_ip);

    return fort_conf_conn_local_allowed(conn, conf_flags);
}

static void fort_callout_ale_classify_conn(PFORT_CALLOUT_ARG ca)
{
    FORT_CHECK_STACK(FORT_CALLOUT_ALE_CLASSIFY);

    const UINT32 classify_flags = ca->inFixedValues->incomingValue[ca->fi->flags].value.uint32;

    FORT_CALLOUT_ALE_EXTRA cx = {
        .conn = {
            .inbound = ca->inbound,
            .isIPv6 = ca->isIPv6,
            .is_loopback = (classify_flags & FWP_CONDITION_FLAG_IS_LOOPBACK) != 0,
            .is_reauth = (classify_flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE) != 0,
        },
    };

    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;
    const FORT_CONF_FLAGS conf_flags = fort_device_conf_flags(device_conf);

    if (fort_callout_ale_is_local_address(ca, &cx, conf_flags)) {
        fort_callout_ale_classify_permit(ca, &cx.conn);
        return;
    }

    fort_callout_ale_by_conf(ca, &cx, device_conf, conf_flags);
}

typedef struct fort_callout_ale_layer
{
    UCHAR inbound : 1;
    UCHAR isIPv6 : 1;

    FORT_CALLOUT_FIELD_INDEX fi;
} FORT_CALLOUT_ALE_LAYER, *PFORT_CALLOUT_ALE_LAYER;

typedef const FORT_CALLOUT_ALE_LAYER *PCFORT_CALLOUT_ALE_LAYER;

static const FORT_CALLOUT_ALE_LAYER g_aleConnectV4 = {
    .inbound = FALSE,
    .isIPv6 = FALSE,
    .fi = {
        .flags = FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL,
        .profileId = FWPS_FIELD_ALE_AUTH_CONNECT_V4_ORIGINAL_PROFILE_ID,
    },
};

static const FORT_CALLOUT_ALE_LAYER g_aleConnectV6 = {
    .inbound = FALSE,
    .isIPv6 = TRUE,
    .fi = {
        .flags = FWPS_FIELD_ALE_AUTH_CONNECT_V6_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_PROTOCOL,
        .profileId = FWPS_FIELD_ALE_AUTH_CONNECT_V6_ORIGINAL_PROFILE_ID,
    },
};

static const FORT_CALLOUT_ALE_LAYER g_aleAcceptV4 = {
    .inbound = TRUE,
    .isIPv6 = FALSE,
    .fi = {
        .flags = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL,
        .profileId = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ORIGINAL_PROFILE_ID,
    },
};

static const FORT_CALLOUT_ALE_LAYER g_aleAcceptV6 = {
    .inbound = TRUE,
    .isIPv6 = TRUE,
    .fi = {
        .flags = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_PROTOCOL,
        .profileId = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_ORIGINAL_PROFILE_ID,
    },
};

inline static PCFORT_CALLOUT_ALE_LAYER fort_callout_ale_layer(UINT16 layerId)
{
    switch (layerId) {
    case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
        return &g_aleConnectV4;
    case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
        return &g_aleConnectV6;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
        return &g_aleAcceptV4;
    default:
        return &g_aleAcceptV6;
    }
}

// @CodeScene(disable:"Excess Number of Function Arguments")
FORT_API void NTAPI fort_callout_ale_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    PCFORT_CALLOUT_ALE_LAYER layer = fort_callout_ale_layer(inFixedValues->layerId);

    FORT_CALLOUT_ARG ca = {
        .fi = &layer->fi,
        .inFixedValues = inFixedValues,
        .inMetaValues = inMetaValues,
        .netBufList = layerData,
        .filter = filter,
        .classifyOut = classifyOut,
        .flowContext = flowContext,
        .inbound = layer->inbound,
        .isIPv6 = layer->isIPv6,
    };

    fort_callout_ale_classify_conn(&ca);
}
