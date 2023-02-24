/* Fort Firewall Callouts */

#include "fortcout.h"

#include "common/fortdef.h"
#include "common/fortioctl.h"
#include "common/fortprov.h"

#include "fortcoutarg.h"
#include "fortdev.h"
#include "fortps.h"
#include "forttrace.h"
#include "fortutl.h"

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

inline static BOOL fort_callout_classify_blocked_log_stat(FORT_CALLOUT_ARG ca,
        FORT_CALLOUT_ALE_INDEX ci, PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref,
        FORT_APP_FLAGS app_flags)
{
    const UINT64 flow_id = ca.inMetaValues->flowHandle;

    const IPPROTO ip_proto = (IPPROTO) ca.inFixedValues->incomingValue[ci.ipProto].value.uint8;
    const BOOL is_tcp = (ip_proto == IPPROTO_TCP);

    const UCHAR group_index = app_flags.group_index;
    const BOOL is_reauth = (cx->classify_flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE) != 0;

    BOOL is_new_proc = FALSE;

    const NTSTATUS status = fort_flow_associate(&fort_device()->stat, flow_id, cx->process_id,
            group_index, ca.isIPv6, is_tcp, ca.inbound, is_reauth, &is_new_proc);

    if (!NT_SUCCESS(status)) {
        if (status == FORT_STATUS_FLOW_BLOCK) {
            cx->block_reason = FORT_BLOCK_REASON_REAUTH;
            return TRUE; /* block (Reauth) */
        }

        LOG("Classify v4: Flow assoc. error: %x\n", status);
        TRACE(FORT_CALLOUT_FLOW_ASSOC_ERROR, status, 0, 0);
    } else if (is_new_proc) {
        fort_buffer_proc_new_write(&fort_device()->buffer, cx->process_id, cx->real_path->Length,
                cx->real_path->Buffer, &cx->irp, &cx->info);
    }

    return FALSE;
}

inline static void fort_callout_classify_blocked_log_path(PFORT_CALLOUT_ALE_EXTRA cx,
        PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags, FORT_APP_FLAGS app_flags)
{
    if (app_flags.v == 0 && (conf_flags.allow_all_new || conf_flags.log_blocked)
            && conf_flags.filter_enabled) {
        app_flags.blocked = (UCHAR) cx->blocked;
        app_flags.alerted = 1;
        app_flags.is_new = 1;

        if (NT_SUCCESS(fort_conf_ref_exe_add_path(
                    conf_ref, cx->path->Buffer, cx->path->Length, app_flags))) {
            fort_buffer_blocked_write(&fort_device()->buffer, cx->blocked, cx->process_id,
                    cx->real_path->Length, cx->real_path->Buffer, &cx->irp, &cx->info);
        }
    }
}

inline static void fort_callout_classify_blocked_log_ip(FORT_CALLOUT_ARG ca,
        FORT_CALLOUT_ALE_INDEX ci, PFORT_CALLOUT_ALE_EXTRA cx, FORT_CONF_FLAGS conf_flags)
{
    if (cx->block_reason != FORT_BLOCK_REASON_UNKNOWN && conf_flags.log_blocked_ip) {
        const UINT32 *local_ip = ca.isIPv6
                ? (const UINT32 *) ca.inFixedValues->incomingValue[ci.localIp].value.byteArray16
                : &ca.inFixedValues->incomingValue[ci.localIp].value.uint32;

        const UINT16 local_port = ca.inFixedValues->incomingValue[ci.localPort].value.uint16;
        const UINT16 remote_port = ca.inFixedValues->incomingValue[ci.remotePort].value.uint16;
        const IPPROTO ip_proto = (IPPROTO) ca.inFixedValues->incomingValue[ci.ipProto].value.uint8;

        fort_buffer_blocked_ip_write(&fort_device()->buffer, ca.isIPv6, ca.inbound, cx->inherited,
                cx->block_reason, ip_proto, local_port, remote_port, local_ip, cx->remote_ip,
                cx->process_id, cx->real_path->Length, cx->real_path->Buffer, &cx->irp, &cx->info);
    }
}

inline static void fort_callout_classify_blocked_log(FORT_CALLOUT_ARG ca, FORT_CALLOUT_ALE_INDEX ci,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref, FORT_CONF_FLAGS conf_flags)
{
    FORT_APP_FLAGS app_flags = fort_conf_app_find(
            &conf_ref->conf, cx->path->Buffer, cx->path->Length, fort_conf_exe_find);

    if (!cx->blocked /* collect traffic, when Filter Disabled */
            || (app_flags.v == 0 && conf_flags.allow_all_new) /* collect new Blocked Programs */
            || !fort_conf_app_blocked(&conf_ref->conf, app_flags, &cx->block_reason)) {
        if (conf_flags.log_stat
                && fort_callout_classify_blocked_log_stat(ca, ci, cx, conf_ref, app_flags)) {
            cx->blocked = TRUE; /* blocked */
            return;
        }

        cx->blocked = FALSE; /* allow */
    }

    fort_callout_classify_blocked_log_path(cx, conf_ref, conf_flags, app_flags);
}

inline static BOOL fort_callout_classify_blocked_filter_flags(FORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, FORT_CONF_FLAGS conf_flags, PFORT_CONF_REF conf_ref)
{
    if (conf_flags.stop_traffic) {
        cx->blocked = TRUE; /* block all */
        return TRUE;
    }

    if (!fort_conf_ip_is_inet(&conf_ref->conf,
                (fort_conf_zones_ip_included_func *) fort_conf_zones_ip_included,
                &fort_device()->conf, cx->remote_ip, ca.isIPv6)) {
        cx->blocked = FALSE; /* allow LocalNetwork */
        return TRUE;
    }

    if (conf_flags.stop_inet_traffic) {
        cx->blocked = TRUE; /* block Internet */
        return TRUE;
    }

    if (!fort_conf_ip_inet_included(&conf_ref->conf,
                (fort_conf_zones_ip_included_func *) fort_conf_zones_ip_included,
                &fort_device()->conf, cx->remote_ip, ca.isIPv6)) {
        cx->block_reason = FORT_BLOCK_REASON_IP_INET;
        cx->blocked = TRUE; /* block address */
        return TRUE;
    }

    return FALSE;
}

inline static BOOL fort_callout_classify_blocked_flags(FORT_CALLOUT_ARG ca,
        PFORT_CALLOUT_ALE_EXTRA cx, FORT_CONF_FLAGS conf_flags, PFORT_CONF_REF conf_ref)
{
    if (conf_flags.filter_enabled) {
        return fort_callout_classify_blocked_filter_flags(ca, cx, conf_flags, conf_ref);
    }

    cx->blocked = FALSE;

    if (!(conf_flags.log_stat && conf_flags.log_stat_no_filter))
        return TRUE; /* allow (Filter Disabled) */

    return FALSE;
}

inline static void fort_callout_classify_check_conf(FORT_CALLOUT_ARG ca, FORT_CALLOUT_ALE_INDEX ci,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_CONF_REF conf_ref)
{
    const FORT_CONF_FLAGS conf_flags = conf_ref->conf.flags;

    const UINT32 process_id = (UINT32) ca.inMetaValues->processId;

    UNICODE_STRING real_path;
    real_path.Length = (UINT16) (ca.inMetaValues->processPath->size
            - sizeof(WCHAR)); /* chop terminating zero */
    real_path.MaximumLength = real_path.Length;
    real_path.Buffer = (PWSTR) ca.inMetaValues->processPath->data;

    BOOL inherited = FALSE;
    UNICODE_STRING path;
    if (!fort_pstree_get_proc_name(&fort_device()->ps_tree, process_id, &path, &inherited)) {
        path = real_path;
    } else if (!inherited) {
        real_path = path;
    }

    cx->process_id = process_id;
    cx->path = &path;
    cx->real_path = &real_path;
    cx->inherited = inherited;

    cx->blocked = TRUE;
    cx->block_reason = FORT_BLOCK_REASON_UNKNOWN;

    if (!fort_callout_classify_blocked_flags(ca, cx, conf_flags, conf_ref)) {
        fort_callout_classify_blocked_log(ca, ci, cx, conf_ref, conf_flags);
    }

    if (cx->blocked) {
        /* Log the blocked connection */
        fort_callout_classify_blocked_log_ip(ca, ci, cx, conf_flags);

        /* Block the connection */
        fort_callout_classify_block(ca.classifyOut);
    } else {
        if (cx->block_reason == FORT_BLOCK_REASON_NONE) {
            /* Continue the search */
            fort_callout_classify_continue(ca.classifyOut);
        } else {
            /* Allow the connection */
            fort_callout_classify_permit(ca.filter, ca.classifyOut);
        }
    }
}

inline static void fort_callout_classify_by_conf(FORT_CALLOUT_ARG ca, FORT_CALLOUT_ALE_INDEX ci,
        PFORT_CALLOUT_ALE_EXTRA cx, PFORT_DEVICE_CONF device_conf)
{
    PFORT_CONF_REF conf_ref = fort_conf_ref_take(device_conf);

    if (conf_ref == NULL) {
        if (fort_device_flag(device_conf, FORT_DEVICE_PROV_BOOT) != 0) {
            fort_callout_classify_block(ca.classifyOut);
        } else {
            fort_callout_classify_continue(ca.classifyOut);
        }
        return;
    }

    cx->irp = NULL;

    fort_callout_classify_check_conf(ca, ci, cx, conf_ref);

    fort_conf_ref_put(device_conf, conf_ref);

    if (cx->irp != NULL) {
        fort_request_complete_info(cx->irp, STATUS_SUCCESS, cx->info);
    }
}

static void fort_callout_classify(FORT_CALLOUT_ARG ca, FORT_CALLOUT_ALE_INDEX ci)
{
    const UINT32 classify_flags = ca.inFixedValues->incomingValue[ci.flags].value.uint32;

    const BOOL is_reauth = (classify_flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE) != 0;
    if (is_reauth) {
        ca.inbound = (ca.inMetaValues->packetDirection == FWP_DIRECTION_INBOUND);
    }

    const UINT32 *remote_ip = ca.isIPv6
            ? (const UINT32 *) ca.inFixedValues->incomingValue[ci.remoteIp].value.byteArray16
            : &ca.inFixedValues->incomingValue[ci.remoteIp].value.uint32;

    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;

    if (!device_conf->conf_flags.filter_locals
            && ((classify_flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
                    || fort_addr_is_local_broadcast(remote_ip, ca.isIPv6))) {
        fort_callout_classify_permit(ca.filter, ca.classifyOut);
        return;
    }

    FORT_CALLOUT_ALE_EXTRA cx = {
        .classify_flags = classify_flags,
        .remote_ip = remote_ip,
    };

    fort_callout_classify_by_conf(ca, ci, &cx, device_conf);
}

static void NTAPI fort_callout_connect_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
        .isIPv6 = FALSE,
    };

    FORT_CALLOUT_ALE_INDEX ci = {
        .flags = FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL,
    };

    fort_callout_classify(ca, ci);
}

static void NTAPI fort_callout_connect_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
        .isIPv6 = TRUE,
    };

    FORT_CALLOUT_ALE_INDEX ci = {
        .flags = FWPS_FIELD_ALE_AUTH_CONNECT_V6_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_PROTOCOL,
    };

    fort_callout_classify(ca, ci);
}

static void NTAPI fort_callout_accept_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
        .isIPv6 = FALSE,
    };

    FORT_CALLOUT_ALE_INDEX ci = {
        .flags = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL,
    };

    fort_callout_classify(ca, ci);
}

static void NTAPI fort_callout_accept_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
        .isIPv6 = TRUE,
    };

    FORT_CALLOUT_ALE_INDEX ci = {
        .flags = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_FLAGS,
        .localIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_ADDRESS,
        .remoteIp = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS,
        .localPort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_PORT,
        .remotePort = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_PORT,
        .ipProto = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_PROTOCOL,
    };

    fort_callout_classify(ca, ci);
}

static NTSTATUS NTAPI fort_callout_notify(
        FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID *filterKey, FWPS_FILTER0 *filter)
{
    UNUSED(notifyType);
    UNUSED(filterKey);
    UNUSED(filter);

    return STATUS_SUCCESS;
}

inline static void fort_callout_flow_classify(FORT_CALLOUT_ARG ca, UINT32 dataSize)
{
    const UINT32 headerSize = ca.inbound ? ca.inMetaValues->transportHeaderSize : 0;

    fort_flow_classify(&fort_device()->stat, ca.flowContext, headerSize + dataSize, ca.inbound);
}

static void NTAPI fort_callout_stream_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    FWPS_STREAM_CALLOUT_IO_PACKET0 *packet = layerData;

    const FWPS_STREAM_DATA0 *streamData = packet->streamData;
    const UINT32 streamFlags = streamData->flags;
    const UINT32 dataSize = (UINT32) streamData->dataLength;

    const BOOL inbound = (streamFlags & FWPS_STREAM_FLAG_RECEIVE) != 0;

    FORT_CALLOUT_ARG ca = {
        .inFixedValues = inFixedValues,
        .inMetaValues = inMetaValues,
        .packet = layerData,
        .filter = filter,
        .flowContext = flowContext,
        .classifyOut = classifyOut,
        .inbound = inbound,
    };

    fort_callout_flow_classify(ca, dataSize);

    fort_callout_classify_permit(filter, classifyOut);
}

static void fort_callout_datagram_classify(FORT_CALLOUT_ARG ca, FORT_CALLOUT_DATAGRAM_INDEX ci)
{
    const PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(ca.netBufList);
    const UINT32 dataSize = NET_BUFFER_DATA_LENGTH(netBuf);

    const FWP_DIRECTION direction =
            (FWP_DIRECTION) ca.inFixedValues->incomingValue[ci.direction].value.uint8;
    ca.inbound = (direction == FWP_DIRECTION_INBOUND);

    fort_callout_flow_classify(ca, dataSize);

    fort_callout_classify_permit(ca.filter, ca.classifyOut);
}

static void NTAPI fort_callout_datagram_classify_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
    };

    FORT_CALLOUT_DATAGRAM_INDEX ci = {
        .direction = FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION,
    };

    fort_callout_datagram_classify(ca, ci);
}

static void NTAPI fort_callout_datagram_classify_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
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
    };

    FORT_CALLOUT_DATAGRAM_INDEX ci = {
        .direction = FWPS_FIELD_DATAGRAM_DATA_V6_DIRECTION,
    };

    fort_callout_datagram_classify(ca, ci);
}

static void NTAPI fort_callout_flow_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);

    PFORT_STAT stat = &fort_device()->stat;

    if ((fort_stat_flags(stat) & FORT_STAT_CLOSED) != 0)
        return;

    fort_shaper_drop_flow_packets(&fort_device()->shaper, flowContext);

    fort_flow_delete(stat, flowContext);
}

static BOOL fort_callout_transport_is_ipsec_detunneled(
        const PNET_BUFFER_LIST netBufList, BOOL inbound)
{
    /* To be compatible with Vista's IpSec implementation, we must not
     * intercept not-yet-detunneled IpSec traffic. */

    if (!inbound)
        return TRUE;

    FWPS_PACKET_LIST_INFORMATION0 packet_info;
    RtlZeroMemory(&packet_info, sizeof(FWPS_PACKET_LIST_INFORMATION0));

    FwpsGetPacketListSecurityInformation0(netBufList,
            FWPS_PACKET_LIST_INFORMATION_QUERY_IPSEC | FWPS_PACKET_LIST_INFORMATION_QUERY_INBOUND,
            &packet_info);

    return !packet_info.ipsecInformation.inbound.isTunnelMode
            || packet_info.ipsecInformation.inbound.isDeTunneled;
}

static void fort_callout_transport_classify(FORT_CALLOUT_ARG ca)
{
    if ((ca.classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0
            || ca.classifyOut->actionType == FWP_ACTION_BLOCK)
        return; /* Can't act on the packet */

    if (!FWPS_IS_METADATA_FIELD_PRESENT(ca.inMetaValues, FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED)
            && ca.netBufList != NULL
            && fort_callout_transport_is_ipsec_detunneled(ca.netBufList, ca.inbound)
            /* Process the Packet by Shaper */
            && fort_shaper_packet_process(&fort_device()->shaper, ca)) {

        fort_callout_classify_drop(ca.classifyOut); /* drop */
        return;
    }

    fort_callout_classify_permit(ca.filter, ca.classifyOut); /* permit */
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

    fort_callout_transport_classify(ca);
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

    fort_callout_transport_classify(ca);
}

static void NTAPI fort_callout_transport_delete(
        UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);
    UNUSED(flowContext);
}

inline static NTSTATUS fort_callout_register(
        PDEVICE_OBJECT device, const FWPS_CALLOUT0 *callouts, const PUINT32 *calloutIds, int count)
{
    for (int i = 0; i < count; ++i) {
        const NTSTATUS status = FwpsCalloutRegister0(device, &callouts[i], calloutIds[i]);
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
    const FWPS_CALLOUT0 callouts[] = {
        /* IPv4 connect callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_CONNECT_V4,
                .classifyFn = &fort_callout_connect_v4,
                .notifyFn = &fort_callout_notify,
        },
        /* IPv6 connect callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_CONNECT_V6,
                .classifyFn = &fort_callout_connect_v6,
                .notifyFn = &fort_callout_notify,
        },
        /* IPv4 accept callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4,
                .classifyFn = &fort_callout_accept_v4,
                .notifyFn = &fort_callout_notify,
        },
        /* IPv6 accept callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_ACCEPT_V6,
                .classifyFn = &fort_callout_accept_v6,
                .notifyFn = &fort_callout_notify,
        },
    };

    const PUINT32 calloutIds[] = {
        &stat->connect4_id,
        &stat->connect6_id,
        &stat->accept4_id,
        &stat->accept6_id,
    };

    return fort_callout_register(
            device, callouts, calloutIds, /*count=*/sizeof(callouts) / sizeof(callouts[0]));
}

static NTSTATUS fort_callout_install_stream(PDEVICE_OBJECT device, PFORT_STAT stat)
{
    const FWPS_CALLOUT0 callouts[] = {
        /* IPv4 stream callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_STREAM_V4,
                .classifyFn = &fort_callout_stream_classify,
                .notifyFn = &fort_callout_notify,
                .flowDeleteFn = &fort_callout_flow_delete,
                .flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        },
        /* IPv6 stream callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_STREAM_V6,
                .classifyFn = &fort_callout_stream_classify,
                .notifyFn = &fort_callout_notify,
                .flowDeleteFn = &fort_callout_flow_delete,
                .flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        },
        /* IPv4 datagram callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V4,
                .classifyFn = &fort_callout_datagram_classify_v4,
                .notifyFn = &fort_callout_notify,
                .flowDeleteFn = &fort_callout_flow_delete,
                .flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        },
        /* IPv6 datagram callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V6,
                .classifyFn = &fort_callout_datagram_classify_v6,
                .notifyFn = &fort_callout_notify,
                .flowDeleteFn = &fort_callout_flow_delete,
                .flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        },
    };

    const PUINT32 calloutIds[] = {
        &stat->stream4_id,
        &stat->stream6_id,
        &stat->datagram4_id,
        &stat->datagram6_id,
    };

    return fort_callout_register(
            device, callouts, calloutIds, /*count=*/sizeof(callouts) / sizeof(callouts[0]));
}

static NTSTATUS fort_callout_install_transport(PDEVICE_OBJECT device, PFORT_STAT stat)
{
    const FWPS_CALLOUT0 callouts[] = {
        /* IPv4 inbound transport callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V4,
                .classifyFn = &fort_callout_transport_classify_in,
                .notifyFn = &fort_callout_notify,
                .flowDeleteFn = &fort_callout_transport_delete,
                .flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        },
        /* IPv6 inbound transport callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V6,
                .classifyFn = &fort_callout_transport_classify_in,
                .notifyFn = &fort_callout_notify,
                .flowDeleteFn = &fort_callout_transport_delete,
                .flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        },
        /* IPv4 outbound transport callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V4,
                .classifyFn = &fort_callout_transport_classify_out,
                .notifyFn = &fort_callout_notify,
                .flowDeleteFn = &fort_callout_transport_delete,
                .flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        },
        /* IPv6 outbound transport callout */
        {
                .calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V6,
                .classifyFn = &fort_callout_transport_classify_out,
                .notifyFn = &fort_callout_notify,
                .flowDeleteFn = &fort_callout_transport_delete,
                .flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        },
    };

    const PUINT32 calloutIds[] = {
        &stat->in_transport4_id,
        &stat->in_transport6_id,
        &stat->out_transport4_id,
        &stat->out_transport6_id,
    };

    return fort_callout_register(
            device, callouts, calloutIds, /*count=*/sizeof(callouts) / sizeof(callouts[0]));
}

FORT_API NTSTATUS fort_callout_install(PDEVICE_OBJECT device)
{
    PFORT_STAT stat = &fort_device()->stat;

    NTSTATUS status;
    if (!NT_SUCCESS(status = fort_callout_install_ale(device, stat))
            || !NT_SUCCESS(status = fort_callout_install_stream(device, stat))
            || !NT_SUCCESS(status = fort_callout_install_transport(device, stat))) {
        return status;
    }

    return STATUS_SUCCESS;
}

FORT_API void fort_callout_remove(void)
{
    PFORT_STAT stat = &fort_device()->stat;

    UINT32 *const calloutIds[] = {
        &stat->connect4_id,
        &stat->connect6_id,
        &stat->accept4_id,
        &stat->accept6_id,
        &stat->stream4_id,
        &stat->stream6_id,
        &stat->datagram4_id,
        &stat->datagram6_id,
        &stat->in_transport4_id,
        &stat->in_transport6_id,
        &stat->out_transport4_id,
        &stat->out_transport6_id,
    };

    for (int i = 0; i < sizeof(calloutIds) / sizeof(calloutIds[0]); ++i) {
        UINT32 *calloutId = calloutIds[i];
        FwpsCalloutUnregisterById0(*calloutId);
        *calloutId = 0;
    }
}

inline static NTSTATUS fort_callout_force_reauth_prov_check_flow_filter(HANDLE engine,
        const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags, BOOL force)
{
    NTSTATUS status = STATUS_SUCCESS;

    const PFORT_CONF_GROUP conf_group = &fort_device()->stat.conf_group;
    const UINT16 limit_bits = conf_group->limit_bits;

    const BOOL old_filter_packets =
            fort_device_flag(&fort_device()->conf, FORT_DEVICE_FILTER_PACKETS) != 0;
    const BOOL filter_packets =
            conf_flags.filter_enabled && (conf_flags.group_bits & limit_bits) != 0;

    if (force || old_conf_flags.log_stat != conf_flags.log_stat
            || old_filter_packets != filter_packets) {
        fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_FILTER_PACKETS, filter_packets);

        fort_prov_flow_unregister(engine);

        if (conf_flags.log_stat) {
            status = fort_prov_flow_register(engine, filter_packets);
        }
    }

    return status;
}

static NTSTATUS fort_callout_force_reauth_prov(
        HANDLE engine, const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags)
{
    NTSTATUS status;

    /* Check provider filters */
    BOOL prov_recreated = FALSE;
    if (old_conf_flags.prov_boot != conf_flags.prov_boot) {
        fort_prov_unregister(engine);

        status = fort_prov_register(engine, conf_flags.prov_boot);
        if (status)
            return status;

        prov_recreated = TRUE;
    }

    /* Check flow filter */
    status = fort_callout_force_reauth_prov_check_flow_filter(engine, old_conf_flags, conf_flags,
            /*force=*/prov_recreated);
    if (status)
        return status;

    /* Force reauth filter */
    return fort_prov_reauth(engine);
}

FORT_API NTSTATUS fort_callout_force_reauth(const FORT_CONF_FLAGS old_conf_flags)
{
    NTSTATUS status;

    fort_timer_set_running(&fort_device()->log_timer, /*run=*/FALSE);

    /* Check app group periods & update group_bits */
    {
        int periods_n = 0;

        fort_conf_ref_period_update(&fort_device()->conf, /*force=*/TRUE, &periods_n);

        fort_timer_set_running(&fort_device()->app_timer, /*run=*/(periods_n != 0));
    }

    const FORT_CONF_FLAGS conf_flags = fort_device()->conf.conf_flags;

    /* Handle log_stat */
    fort_stat_log_update(&fort_device()->stat, conf_flags.log_stat);

    /* Open provider */
    HANDLE engine;
    status = fort_prov_open(&engine);
    if (NT_SUCCESS(status)) {
        fort_prov_trans_begin(engine);

        status = fort_callout_force_reauth_prov(engine, old_conf_flags, conf_flags);

        status = fort_prov_trans_close(engine, status);
    }

    if (NT_SUCCESS(status)) {
        const BOOL log_enabled = (conf_flags.allow_all_new || conf_flags.log_blocked
                || conf_flags.log_stat || conf_flags.log_blocked_ip);

        fort_timer_set_running(&fort_device()->log_timer, /*run=*/log_enabled);
    } else {
        LOG("Callout Reauth: Error: %x\n", status);
        TRACE(FORT_CALLOUT_CALLOUT_REAUTH_ERROR, status, 0, 0);
    }

    return status;
}

inline static void fort_callout_timer_update_system_time(
        PFORT_STAT stat, PFORT_BUFFER buf, PIRP *irp, ULONG_PTR *info)
{
    LARGE_INTEGER system_time;
    PCHAR out;

    KeQuerySystemTime(&system_time);

    if (stat->system_time.QuadPart != system_time.QuadPart
            && NT_SUCCESS(fort_buffer_prepare(buf, FORT_LOG_TIME_SIZE, &out, irp, info))) {
        const INT64 unix_time = fort_system_to_unix_time(system_time.QuadPart);

        const UCHAR old_stat_flags = fort_stat_flags_set(stat, FORT_STAT_TIME_CHANGED, FALSE);
        const BOOL time_changed = ((old_stat_flags & FORT_STAT_TIME_CHANGED) != 0);

        stat->system_time = system_time;

        fort_log_time_write(out, time_changed, unix_time);
    }
}

inline static void fort_callout_timer_flush_stat_traf(
        PFORT_STAT stat, PFORT_BUFFER buf, PIRP *irp, ULONG_PTR *info)
{
    while (stat->proc_active_count != 0) {
        const UINT16 proc_count = (stat->proc_active_count < FORT_LOG_STAT_BUFFER_PROC_COUNT)
                ? stat->proc_active_count
                : FORT_LOG_STAT_BUFFER_PROC_COUNT;
        const UINT32 len = FORT_LOG_STAT_SIZE(proc_count);
        PCHAR out;
        NTSTATUS status;

        status = fort_buffer_prepare(buf, len, &out, irp, info);
        if (!NT_SUCCESS(status)) {
            LOG("Callout Timer: Error: %x\n", status);
            TRACE(FORT_CALLOUT_CALLOUT_TIMER_ERROR, status, 0, 0);
            break;
        }

        fort_log_stat_traf_header_write(out, proc_count);
        out += FORT_LOG_STAT_HEADER_SIZE;

        fort_stat_dpc_traf_flush(stat, proc_count, out);
    }
}

FORT_API void NTAPI fort_callout_timer(void)
{
    PFORT_BUFFER buf = &fort_device()->buffer;
    PFORT_STAT stat = &fort_device()->stat;

    KLOCK_QUEUE_HANDLE buf_lock_queue;
    KLOCK_QUEUE_HANDLE stat_lock_queue;

    PIRP irp = NULL;
    ULONG_PTR info;

    /* Lock buffer */
    fort_buffer_dpc_begin(buf, &buf_lock_queue);

    /* Lock stat */
    fort_stat_dpc_begin(stat, &stat_lock_queue);

    /* Get current Unix time */
    fort_callout_timer_update_system_time(stat, buf, &irp, &info);

    /* Flush traffic statistics */
    fort_callout_timer_flush_stat_traf(stat, buf, &irp, &info);

    /* Unlock stat */
    fort_stat_dpc_end(&stat_lock_queue);

    /* Flush pending buffer */
    if (irp == NULL) {
        fort_buffer_dpc_flush_pending(buf, &irp, &info);
    }

    /* Unlock buffer */
    fort_buffer_dpc_end(&buf_lock_queue);

    if (irp != NULL) {
        fort_request_complete_info(irp, STATUS_SUCCESS, info);
    }
}
