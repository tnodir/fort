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

static FORT_APP_DATA fort_callout_ale_conf_app_data(
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref)
{
    if (cx->app_data_found)
        return cx->app_data;

    const FORT_APP_DATA app_data = fort_conf_app_find(
            &conf_ref->conf, cx->path->Buffer, cx->path->Length, fort_conf_exe_find, conf_ref);

    fort_callout_ale_set_app_flags(cx, app_data);

    return app_data;
}

inline static BOOL fort_callout_ale_associate_flow(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, FORT_APP_FLAGS app_flags)
{
    const UINT64 flow_id = ca->inMetaValues->flowHandle;

    const IPPROTO ip_proto =
            (IPPROTO) ca->inFixedValues->incomingValue[ca->fi->ipProto].value.uint8;
    const BOOL is_tcp = (ip_proto == IPPROTO_TCP);

    const UCHAR group_index = (UCHAR) app_flags.group_index;

    BOOL log_stat = FALSE;

    const NTSTATUS status = fort_flow_associate(&fort_device()->stat, flow_id, cx->process_id,
            group_index, ca->isIPv6, is_tcp, ca->inbound, cx->is_reauth, &log_stat);

    if (!NT_SUCCESS(status)) {
        if (status != FORT_STATUS_FLOW_BLOCK) {
            LOG("Classify v4: Flow assoc. error: %x\n", status);
            TRACE(FORT_CALLOUT_FLOW_ASSOC_ERROR, status, 0, 0);
        }

        cx->block_reason = FORT_BLOCK_REASON_REAUTH;
        return TRUE; /* block (Error) */
    }

    if (!log_stat) {
        fort_buffer_proc_new_write(&fort_device()->buffer, cx->process_id, cx->real_path->Length,
                cx->real_path->Buffer, &cx->irp, &cx->info);
    }

    return FALSE;
}

inline static BOOL fort_callout_ale_log_app_path_check(
        FORT_CONF_FLAGS conf_flags, FORT_APP_FLAGS app_flags)
{
    return app_flags.v == 0 && conf_flags.filter_enabled
            && (conf_flags.allow_all_new || conf_flags.log_blocked);
}

inline static void fort_callout_ale_log_app_path(PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    if (cx->ignore || !fort_callout_ale_log_app_path_check(conf_flags, app_data.flags))
        return;

    app_data.flags.log_blocked = TRUE;
    app_data.flags.log_conn = TRUE;
    app_data.flags.blocked = (UCHAR) cx->blocked;
    app_data.flags.alerted = TRUE;
    app_data.flags.is_new = TRUE;

    FORT_APP_ENTRY app_entry = {
        .app_data = app_data,
        .path_len = cx->path->Length,
    };

    if (!NT_SUCCESS(fort_conf_ref_exe_add_path(conf_ref, &app_entry, cx->path->Buffer)))
        return;

    fort_callout_ale_set_app_flags(cx, app_data);

    fort_buffer_blocked_write(&fort_device()->buffer, cx->blocked, cx->process_id,
            cx->real_path->Length, cx->real_path->Buffer, &cx->irp, &cx->info);
}

inline static BOOL fort_callout_ale_log_blocked_ip_check_app(
        FORT_CONF_FLAGS conf_flags, FORT_APP_FLAGS app_flags)
{
    return (app_flags.v == 0 || app_flags.log_blocked)
            && (app_flags.alerted || !conf_flags.log_alerted_blocked_ip);
}

inline static BOOL fort_callout_ale_log_blocked_ip_check(
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    if (cx->block_reason == FORT_BLOCK_REASON_UNKNOWN)
        return FALSE;

    if (!(conf_flags.ask_to_connect || conf_flags.log_blocked_ip))
        return FALSE;

    const FORT_APP_DATA app_data = fort_callout_ale_conf_app_data(cx, conf_ref);

    return fort_callout_ale_log_blocked_ip_check_app(conf_flags, app_data.flags);
}

inline static void fort_callout_ale_log_blocked_ip(PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    if (!fort_callout_ale_log_blocked_ip_check(cx, conf_ref, conf_flags))
        return;

    const UINT32 *local_ip = ca->isIPv6
            ? (const UINT32 *) ca->inFixedValues->incomingValue[ca->fi->localIp].value.byteArray16
            : &ca->inFixedValues->incomingValue[ca->fi->localIp].value.uint32;

    const UINT16 local_port = ca->inFixedValues->incomingValue[ca->fi->localPort].value.uint16;
    const UINT16 remote_port = ca->inFixedValues->incomingValue[ca->fi->remotePort].value.uint16;
    const IPPROTO ip_proto =
            (IPPROTO) ca->inFixedValues->incomingValue[ca->fi->ipProto].value.uint8;

    fort_buffer_blocked_ip_write(&fort_device()->buffer, ca->isIPv6, ca->inbound, cx->inherited,
            cx->block_reason, ip_proto, local_port, remote_port, local_ip, cx->remote_ip,
            cx->process_id, cx->real_path->Length, cx->real_path->Buffer, &cx->irp, &cx->info);
}

inline static BOOL fort_callout_ale_add_pending(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, FORT_CONF_FLAGS conf_flags)
{
    if (!fort_pending_add_packet(&fort_device()->pending, ca, cx)) {
        cx->block_reason = FORT_BLOCK_REASON_ASK_LIMIT;
        return TRUE; /* block (error) */
    }

    cx->drop_blocked = TRUE;
    cx->block_reason = FORT_BLOCK_REASON_ASK_PENDING;
    return TRUE; /* drop (pending) */
}

inline static BOOL fort_callout_ale_process_flow(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        FORT_CONF_FLAGS conf_flags, FORT_APP_FLAGS app_flags)
{
    if (app_flags.v == 0 && conf_flags.ask_to_connect) {
        return fort_callout_ale_add_pending(ca, cx, conf_flags);
    }

    if (!conf_flags.log_stat)
        return FALSE;

    return fort_callout_ale_associate_flow(ca, cx, app_flags);
}

static BOOL fort_callout_ale_is_zone_blocked(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, FORT_APP_DATA app_data)
{
    const BOOL app_found = (app_data.flags.v != 0);
    if (!app_found)
        return FALSE;

    if (app_data.flags.lan_only) {
        cx->block_reason = FORT_BLOCK_REASON_LAN_ONLY;
        return TRUE; /* block LAN Only */
    }

    if (app_data.reject_zones != 0
            && fort_conf_zones_ip_included(
                    &fort_device()->conf, app_data.reject_zones, cx->remote_ip, ca->isIPv6)) {
        cx->block_reason = FORT_BLOCK_REASON_ZONE;
        return TRUE; /* block Rejected Zone */
    }

    if (app_data.accept_zones != 0
            && !fort_conf_zones_ip_included(
                    &fort_device()->conf, app_data.accept_zones, cx->remote_ip, ca->isIPv6)) {
        cx->block_reason = FORT_BLOCK_REASON_ZONE;
        return TRUE; /* block Not Accepted Zone */
    }

    return FALSE;
}

inline static BOOL fort_callout_ale_is_new(FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    const BOOL app_found = (app_data.flags.v != 0);

    return !app_found && (conf_flags.allow_all_new || conf_flags.ask_to_connect);
}

inline static BOOL fort_callout_ale_is_allowed(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    /* Collect traffic, when Filter Disabled */
    if (!cx->blocked)
        return TRUE;

    /* "Allow, if not blocked" or "Ask to Connect" */
    if (fort_callout_ale_is_new(conf_flags, app_data))
        return TRUE;

    /* Check LAN Only and Zones */
    if (fort_callout_ale_is_zone_blocked(ca, cx, app_data))
        return FALSE;

    /* Check the conf for a blocked app */
    if (!fort_conf_app_blocked(&conf_ref->conf, app_data.flags, &cx->block_reason))
        return TRUE;

    if (cx->block_reason == FORT_BLOCK_REASON_NONE) {
        cx->ignore = TRUE;
    }

    return FALSE;
}

inline static void fort_callout_ale_log(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    const FORT_APP_DATA app_data = fort_callout_ale_conf_app_data(cx, conf_ref);

    if (fort_callout_ale_is_allowed(ca, cx, conf_ref, conf_flags, app_data)) {

        if (fort_callout_ale_process_flow(ca, cx, conf_flags, app_data.flags))
            return;

        cx->blocked = FALSE; /* allow */
    }

    fort_callout_ale_log_app_path(cx, conf_ref, conf_flags, app_data);
}

inline static BOOL fort_callout_ale_check_filter_flags(PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    if (conf_flags.block_traffic) {
        return TRUE; /* block all */
    }

    if (!fort_conf_ip_is_inet(&conf_ref->conf,
                (fort_conf_zones_ip_included_func *) &fort_conf_zones_ip_included,
                &fort_device()->conf, cx->remote_ip, ca->isIPv6)) {
        cx->blocked = FALSE;
        return TRUE; /* allow LocalNetwork */
    }

    if (conf_flags.block_inet_traffic) {
        return TRUE; /* block Internet */
    }

    if (!fort_conf_ip_inet_included(&conf_ref->conf,
                (fort_conf_zones_ip_included_func *) &fort_conf_zones_ip_included,
                &fort_device()->conf, cx->remote_ip, ca->isIPv6)) {
        cx->block_reason = FORT_BLOCK_REASON_IP_INET;
        return TRUE; /* block address */
    }

    return FALSE;
}

inline static BOOL fort_callout_ale_check_flags(PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    if (conf_flags.filter_enabled) {
        return fort_callout_ale_check_filter_flags(ca, cx, conf_ref, conf_flags);
    }

    cx->blocked = FALSE;

    if (!(conf_flags.log_stat && conf_flags.log_stat_no_filter))
        return TRUE; /* allow (Filter Disabled) */

    return FALSE;
}

inline static void fort_callout_ale_classify_blocked(PCFORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    /* Log the blocked connection */
    fort_callout_ale_log_blocked_ip(ca, cx, conf_ref, conf_flags);

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
    } else if (!cx->blocked) {
        /* Allow the connection */
        fort_callout_classify_permit(ca->filter, ca->classifyOut);
    } else {
        fort_callout_ale_classify_blocked(ca, cx, conf_ref, conf_flags);
    }
}

inline static void fort_callout_ale_check_conf(
        PCFORT_CALLOUT_ARG ca, PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref)
{
    const FORT_CONF_FLAGS conf_flags = conf_ref->conf.flags;

    const UINT32 process_id = (UINT32) ca->inMetaValues->processId;

    UNICODE_STRING real_path;
    real_path.Length = (UINT16) (ca->inMetaValues->processPath->size
            - sizeof(WCHAR)); /* chop terminating zero */
    real_path.MaximumLength = real_path.Length;
    real_path.Buffer = (PWSTR) ca->inMetaValues->processPath->data;

    BOOL isSvcHost = FALSE;
    BOOL inherited = FALSE;
    UNICODE_STRING path;
    if (!fort_pstree_get_proc_name(
                &fort_device()->ps_tree, process_id, &path, &isSvcHost, &inherited)) {
        path = real_path;
    } else if (!inherited) {
        /* TODO: Check "ServiceTag" on Windows 10+ */
#if 0 // !defined(FORT_WIN7_COMPAT)
        PVOID subProcessTag = ca->inMetaValues->subProcessTag;
        if (subProcessTag && isSvcHost) { }
#endif

        real_path = path;
    }

    cx->process_id = process_id;
    cx->path = &path;
    cx->real_path = &real_path;
    cx->inherited = (UCHAR) inherited;

    cx->blocked = TRUE;
    cx->ignore = FALSE;
    cx->block_reason = FORT_BLOCK_REASON_UNKNOWN;

    if (!fort_callout_ale_check_flags(ca, cx, conf_ref, conf_flags)) {
        fort_callout_ale_log(ca, cx, conf_ref, conf_flags);
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

inline static BOOL fort_callout_ale_is_local_address(PFORT_CALLOUT_ARG ca,
        PCFORT_CALLOUT_ALE_EXTRA cx, PFORT_DEVICE_CONF device_conf, const UINT32 classify_flags)
{
    if (fort_device_flag(device_conf, FORT_DEVICE_BOOT_FILTER_LOCALS) != 0)
        return FALSE;

    return ((classify_flags & FWP_CONDITION_FLAG_IS_LOOPBACK) != 0
            || fort_addr_is_local_broadcast(cx->remote_ip, ca->isIPv6));
}

static void fort_callout_ale_classify(PFORT_CALLOUT_ARG ca)
{
    FORT_CHECK_STACK(FORT_CALLOUT_ALE_CLASSIFY);

    const UINT32 classify_flags = ca->inFixedValues->incomingValue[ca->fi->flags].value.uint32;

    const BOOL is_reauth = (classify_flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE) != 0;
    if (is_reauth) {
        ca->inbound = (ca->inMetaValues->packetDirection == FWP_DIRECTION_INBOUND);
    }

    const UINT32 *remote_ip = ca->isIPv6
            ? (const UINT32 *) ca->inFixedValues->incomingValue[ca->fi->remoteIp].value.byteArray16
            : &ca->inFixedValues->incomingValue[ca->fi->remoteIp].value.uint32;

    FORT_CALLOUT_ALE_EXTRA cx = {
        .is_reauth = is_reauth,
        .remote_ip = remote_ip,
    };

    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;

    if (fort_callout_ale_is_local_address(ca, &cx, device_conf, classify_flags)) {
        fort_callout_classify_permit(ca->filter, ca->classifyOut);
        return;
    }

    fort_callout_ale_by_conf(ca, &cx, device_conf);
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
        .flowContext = flowContext,
        .classifyOut = classifyOut,
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

inline static void fort_callout_flow_classify(PCFORT_CALLOUT_ARG ca, UINT32 dataSize)
{
    const UINT32 headerSize = ca->inbound ? ca->inMetaValues->transportHeaderSize : 0;

    fort_flow_classify(&fort_device()->stat, ca->flowContext, headerSize + dataSize, ca->inbound);
}

static void NTAPI fort_callout_stream_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    FORT_CHECK_STACK(FORT_CALLOUT_STREAM_CLASSIFY);

    FWPS_STREAM_CALLOUT_IO_PACKET0 *packet = layerData;

    const FWPS_STREAM_DATA0 *streamData = packet->streamData;
    const UINT32 streamFlags = streamData->flags;
    const UINT32 dataSize = (UINT32) streamData->dataLength;

    const BOOL inbound = (streamFlags & FWPS_STREAM_FLAG_RECEIVE) != 0;

    const FORT_CALLOUT_ARG ca = {
        .inFixedValues = inFixedValues,
        .inMetaValues = inMetaValues,
        .packet = layerData,
        .filter = filter,
        .flowContext = flowContext,
        .classifyOut = classifyOut,
        .inbound = inbound,
    };

    fort_callout_flow_classify(&ca, dataSize);

    fort_callout_classify_permit(filter, classifyOut);
}

static void fort_callout_datagram_classify(PFORT_CALLOUT_ARG ca)
{
    FORT_CHECK_STACK(FORT_CALLOUT_DATAGRAM_CLASSIFY);

    const PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(ca->netBufList);
    const UINT32 dataSize = NET_BUFFER_DATA_LENGTH(netBuf);

    const FWP_DIRECTION direction =
            (FWP_DIRECTION) ca->inFixedValues->incomingValue[ca->fi->direction].value.uint8;
    ca->inbound = (direction == FWP_DIRECTION_INBOUND);

    fort_callout_flow_classify(ca, dataSize);

    fort_callout_classify_permit(ca->filter, ca->classifyOut);
}

inline static void fort_callout_datagram_classify_v(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        PCFORT_CALLOUT_FIELD_INDEX fi, BOOL isIPv6)
{
    FORT_CALLOUT_ARG ca = {
        .fi = fi,
        .inFixedValues = inFixedValues,
        .inMetaValues = inMetaValues,
        .netBufList = layerData,
        .filter = filter,
        .flowContext = flowContext,
        .classifyOut = classifyOut,
        .isIPv6 = isIPv6,
    };

    fort_callout_datagram_classify(&ca);
}

static void NTAPI fort_callout_datagram_classify_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    static const FORT_CALLOUT_FIELD_INDEX fi = {
        .direction = FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION,
    };

    fort_callout_datagram_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*isIPv6=*/FALSE);
}

static void NTAPI fort_callout_datagram_classify_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    static const FORT_CALLOUT_FIELD_INDEX fi = {
        .direction = FWPS_FIELD_DATAGRAM_DATA_V6_DIRECTION,
    };

    fort_callout_datagram_classify_v(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, &fi, /*isIPv6=*/TRUE);
}

static void NTAPI fort_callout_flow_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);

    FORT_CHECK_STACK(FORT_CALLOUT_FLOW_DELETE);

    fort_shaper_drop_flow_packets(&fort_device()->shaper, flowContext);

    fort_flow_delete(&fort_device()->stat, flowContext);
}

inline static BOOL fort_callout_transport_shape(PFORT_CALLOUT_ARG ca)
{
    if (FWPS_IS_METADATA_FIELD_PRESENT(ca->inMetaValues, FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED))
        return FALSE;

    if (ca->netBufList == NULL)
        return FALSE;

    /* Process the Packet by Shaper */
    return fort_shaper_packet_process(&fort_device()->shaper, ca);
}

static void fort_callout_transport_classify(PFORT_CALLOUT_ARG ca)
{
    FORT_CHECK_STACK(FORT_CALLOUT_TRANSPORT_CLASSIFY);

    if ((ca->classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0
            || ca->classifyOut->actionType == FWP_ACTION_BLOCK)
        return; /* Can't act on the packet */

    if (fort_callout_transport_shape(ca)) {
        fort_callout_classify_drop(ca->classifyOut); /* drop */
    } else {
        fort_callout_classify_permit(ca->filter, ca->classifyOut); /* permit */
    }
}

static void NTAPI fort_callout_transport_classify_in(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    FORT_CALLOUT_ARG ca = {
        .inFixedValues = inFixedValues,
        .inMetaValues = inMetaValues,
        .netBufList = layerData,
        .filter = filter,
        .flowContext = flowContext,
        .classifyOut = classifyOut,
        .inbound = TRUE,
    };

    fort_callout_transport_classify(&ca);
}

static void NTAPI fort_callout_transport_classify_out(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    FORT_CALLOUT_ARG ca = {
        .inFixedValues = inFixedValues,
        .inMetaValues = inMetaValues,
        .netBufList = layerData,
        .filter = filter,
        .flowContext = flowContext,
        .classifyOut = classifyOut,
        .inbound = FALSE,
    };

    fort_callout_transport_classify(&ca);
}

static void NTAPI fort_callout_transport_delete(
        UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
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

    /* IPv4 stream callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_STREAM_V4,
            &fort_callout_stream_classify, &fort_callout_flow_delete);
    /* IPv6 stream callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_STREAM_V6,
            &fort_callout_stream_classify, &fort_callout_flow_delete);
    /* IPv4 datagram callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_DATAGRAM_V4,
            &fort_callout_datagram_classify_v4, &fort_callout_flow_delete);
    /* IPv6 datagram callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_DATAGRAM_V6,
            &fort_callout_datagram_classify_v6, &fort_callout_flow_delete);
    /* IPv4 inbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_V4,
            &fort_callout_transport_classify_in, &fort_callout_transport_delete);
    /* IPv6 inbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_V6,
            &fort_callout_transport_classify_in, &fort_callout_transport_delete);
    /* IPv4 outbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_OUT_TRANSPORT_V4,
            &fort_callout_transport_classify_out, &fort_callout_transport_delete);
    /* IPv6 outbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_OUT_TRANSPORT_V6,
            &fort_callout_transport_classify_out, &fort_callout_transport_delete);
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
    const PFORT_CONF_GROUP conf_group = &fort_device()->stat.conf_group;
    const UINT16 limit_bits = conf_group->limit_bits;

    const BOOL old_filter_packets =
            fort_device_flag(&fort_device()->conf, FORT_DEVICE_FILTER_PACKETS) != 0;
    const BOOL filter_packets =
            conf_flags.filter_enabled && (conf_flags.group_bits & limit_bits) != 0;

    const BOOL conf_changed = (old_conf_flags.log_stat != conf_flags.log_stat
            || old_filter_packets != filter_packets);

    if (!force && !conf_changed)
        return STATUS_SUCCESS;

    fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_FILTER_PACKETS, filter_packets);

    fort_prov_flow_unregister(engine);

    if (!conf_flags.log_stat)
        return STATUS_SUCCESS;

    return fort_prov_flow_register(engine, filter_packets);
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

    /* Check app group periods & update group_bits */
    {
        int periods_n = 0;

        fort_conf_ref_period_update(&fort_device()->conf, /*force=*/TRUE, &periods_n);

        fort_timer_set_running(&fort_device()->app_timer, /*run=*/(periods_n != 0));
    }

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
