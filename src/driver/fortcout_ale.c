/* Fort Firewall Callouts: ALE */

#include "fortcout_ale.h"

#include "common/fortconf_conn.h"
#include "common/fortdef.h"

#include "fortcnf_conf.h"
#include "fortcnf_group.h"
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

    FORT_PS_OPT ps_opt = { 0 };

    conn->ps_name =
            fort_pstree_get_proc_name(&fort_device()->ps_tree, conn->process_id, path, &ps_opt);

    if (conn->ps_name != NULL) {

        const BOOL inherited = (ps_opt.flags & FORT_PSNODE_NAME_INHERITED) != 0;
        if (!inherited) {
            *real_path = *path;
            return;
        }

        conn->inherited = TRUE;
    }

    fort_callout_ale_fill_meta_path_real(conn, processPath, ps_opt.path_drive);

    if (!conn->inherited) {
        *path = *real_path;
    }
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
}

static void fort_callout_ale_fill_meta_conn(PCFORT_CALLOUT_ARG ca, PFORT_CONF_META_CONN conn)
{
    conn->flow_id = ca->inMetaValues->flowHandle;

    conn->profile_id = ca->inFixedValues->incomingValue[ca->fi->profileId].value.uint8;

    conn->ip_proto = ca->inFixedValues->incomingValue[ca->fi->ipProto].value.uint8;

    conn->local_port = ca->inFixedValues->incomingValue[ca->fi->localPort].value.uint16;
    conn->remote_port = ca->inFixedValues->incomingValue[ca->fi->remotePort].value.uint16;

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

inline static void fort_callout_ale_associate_flow_log(
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_META_CONN conn)
{
    const NTSTATUS status = fort_buffer_conn_write(
            &fort_device()->buffer, conn, &cx->irp_info, FORT_BUFFER_CONN_WRITE_PROC_NEW);

    /* Log the process again by its next flow */
    if (!NT_SUCCESS(status)) {
        fort_flow_proc_unlog(&fort_device()->stat, conn->process_id);
    }
}

inline static BOOL fort_callout_ale_associate_flow(
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_META_CONN conn)
{
    BOOL proc_stat = FALSE;

    const NTSTATUS status = fort_flow_associate(&fort_device()->stat, conn, &proc_stat);

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

    if (!proc_stat) {
        fort_callout_ale_associate_flow_log(cx, conn);
    }

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

    app_data.flags.log_stat = TRUE;
    app_data.flags.log_allowed_conn = TRUE;
    app_data.flags.log_blocked_conn = TRUE;
    app_data.flags.blocked = !(conf_flags.allow_all_new || conf_flags.app_allow_all);

    app_data.flags.is_new = TRUE;
    app_data.flags.found = TRUE;
    app_data.flags.alerted = TRUE;

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

    /* Conf */
    {
        const BOOL log_conn =
                (conn->act.blocked ? conf_flags.log_blocked_conn : conf_flags.log_allowed_conn);

        if (!(log_conn || conn->ask_to_connect))
            return FALSE;
    }

    /* App */
    {
        const FORT_APP_DATA app_data = fort_callout_ale_conf_app_data(ca, conn, conf_ref);

        return fort_callout_ale_log_conn_check_app(conn, app_data, conf_flags);
    }
}

inline static BOOL fort_callout_ale_add_pending(PCFORT_CALLOUT_ARG ca, PFORT_CONF_META_CONN conn)
{
    if (!fort_pending_add_packet(&fort_device()->pending, ca, conn)) {
        conn->reason = FORT_CONN_REASON_ASK_LIMIT;
        return TRUE; /* block (Error) */
    }

    conn->act.drop_blocked = TRUE;
    conn->reason = FORT_CONN_REASON_ASK_PENDING;
    return TRUE; /* drop (Pending) */
}

inline static BOOL fort_callout_ale_process_flow(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, const FORT_CONF_FLAGS conf_flags)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    if (conn->ask_to_connect) {
        return fort_callout_ale_add_pending(ca, conn);
    }

    if (!conf_flags.log_stat)
        return FALSE;

    return fort_callout_ale_associate_flow(cx, conn);
}

inline static void fort_callout_ale_check_app(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, PCFORT_CONF_CONN_FILTER filter)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    const FORT_CONF_FLAGS conf_flags = filter->conf_flags;

    const FORT_APP_DATA app_data = fort_callout_ale_conf_app_data(ca, conn, conf_ref);

    if (fort_conf_conn_app_allowed(filter, conn, app_data)) {

        if (fort_callout_ale_process_flow(ca, cx, conf_flags)) {
            conn->act.blocked = TRUE; /* block (Error | Pending) */
            return;
        }
    }

    fort_callout_ale_log_app_path(cx, conf_ref, conf_flags, app_data);
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

static BOOL fort_callout_ale_rules_conn_filtered(
        void *ctx, PFORT_CONF_META_CONN conn, UINT16 rule_id)
{
    return fort_devconf_rules_conn_filtered(ctx, conn, rule_id);
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
    .rules_conn_filtered = &fort_callout_ale_rules_conn_filtered,
    .rules_glob_conn_filtered = &fort_callout_ale_rules_glob_conn_filtered,
    .groups_mask_blocked = &fort_callout_ale_groups_mask_blocked,
    .groups_rules_conn_filtered = &fort_callout_ale_groups_rules_conn_filtered,
};

inline static void fort_callout_ale_classify_action(
        PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn)
{
    FWPS_CLASSIFY_OUT0 *classifyOut = ca->classifyOut;

    if (conn->ignore) {
        /* Continue the search */
        fort_callout_classify_continue(classifyOut);
    } else if (conn->act.blocked) {
        if (conn->act.drop_blocked) {
            /* Drop the connection */
            fort_callout_classify_drop(classifyOut);
        } else {
            /* Block the connection */
            fort_callout_classify_block(classifyOut);
        }
    } else {
        /* Allow the connection */
        fort_callout_classify_permit(ca->filter, classifyOut);
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

inline static void fort_callout_ale_check_conf(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, const FORT_CONF_FLAGS conf_flags)
{
    PFORT_CONF_META_CONN conn = &cx->conn;

    fort_callout_ale_fill_meta_conn_proc(ca, conn);
    fort_callout_ale_fill_meta_conn(ca, conn);

    const FORT_CONF_CONN_FILTER filter = {
        .conf_flags = conf_flags,
        .conf = &conf_ref->conf,
        .funcs = &fort_callout_ale_filter_funcs,
        .ctx = &fort_device()->conf,
    };

    if (!fort_conf_conn_flags_filtered(&filter, conn)) {
        fort_callout_ale_check_app(ca, cx, conf_ref, &filter);
    }

    /* Log the connection */
    if (fort_callout_ale_log_conn_check(ca, conn, conf_ref, conf_flags)) {
        fort_buffer_conn_write(
                &fort_device()->buffer, conn, &cx->irp_info, FORT_BUFFER_CONN_WRITE_CONN);
    }

    fort_callout_ale_classify_action(ca, conn);

    /* Free the allocated path */
    fort_path_buffer_free(&conn->path_buf);

    /* Release the process's name */
    fort_pstree_put_proc_name(&fort_device()->ps_tree, conn->ps_name);
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

    fort_callout_fill_meta_ip(ca, ca->fi->remoteIp, &conn->remote_ip);

    return fort_conf_conn_local_allowed(conn, conf_flags);
}

static void fort_callout_ale_classify(PFORT_CALLOUT_ARG ca)
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
        fort_callout_classify_permit(ca->filter, ca->classifyOut);
        return;
    }

    fort_callout_ale_by_conf(ca, &cx, device_conf, conf_flags);
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

FORT_API void NTAPI fort_callout_connect_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
        .profileId = FWPS_FIELD_ALE_AUTH_CONNECT_V4_ORIGINAL_PROFILE_ID,
    };

    fort_callout_ale_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*inbound=*/FALSE, /*isIPv6=*/FALSE);
}

FORT_API void NTAPI fort_callout_connect_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
        .profileId = FWPS_FIELD_ALE_AUTH_CONNECT_V6_ORIGINAL_PROFILE_ID,
    };

    fort_callout_ale_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*inbound=*/FALSE, /*isIPv6=*/TRUE);
}

FORT_API void NTAPI fort_callout_accept_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
        .profileId = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ORIGINAL_PROFILE_ID,
    };

    fort_callout_ale_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*inbound=*/TRUE, /*isIPv6=*/FALSE);
}

FORT_API void NTAPI fort_callout_accept_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
        .profileId = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_ORIGINAL_PROFILE_ID,
    };

    fort_callout_ale_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*inbound=*/TRUE, /*isIPv6=*/TRUE);
}
