/* Fort Firewall Callouts */

#include "fortcout.h"

#include "common/fortdef.h"
#include "common/fortioctl.h"
#include "common/fortprov.h"

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

static BOOL fort_callout_classify_blocked_log_stat(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const FWPS_FILTER0 *filter,
        FWPS_CLASSIFY_OUT0 *classifyOut, int flagsField, int localIpField, int remoteIpField,
        int localPortField, int remotePortField, int ipProtoField, BOOL isIPv6, BOOL inbound,
        UINT32 classify_flags, const UINT32 *remote_ip, FORT_CONF_FLAGS conf_flags,
        UINT32 process_id, PCUNICODE_STRING real_path, PFORT_CONF_REF conf_ref, INT8 *block_reason,
        BOOL blocked, FORT_APP_FLAGS app_flags, PIRP *irp, ULONG_PTR *info)
{
    const UINT64 flow_id = inMetaValues->flowHandle;

    const IPPROTO ip_proto = (IPPROTO) inFixedValues->incomingValue[ipProtoField].value.uint8;
    const BOOL is_tcp = (ip_proto == IPPROTO_TCP);

    const UCHAR group_index = app_flags.group_index;
    const BOOL is_reauth = (classify_flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE);

    BOOL is_new_proc = FALSE;

    const NTSTATUS status = fort_flow_associate(&fort_device()->stat, flow_id, process_id,
            group_index, isIPv6, is_tcp, inbound, is_reauth, &is_new_proc);

    if (!NT_SUCCESS(status)) {
        if (status == FORT_STATUS_FLOW_BLOCK) {
            *block_reason = FORT_BLOCK_REASON_REAUTH;
            return TRUE; /* block (Reauth) */
        }

        LOG("Classify v4: Flow assoc. error: %x\n", status);
        TRACE(FORT_CALLOUT_FLOW_ASSOC_ERROR, status, 0, 0);
    } else if (is_new_proc) {
        fort_buffer_proc_new_write(&fort_device()->buffer, process_id, real_path->Length,
                real_path->Buffer, irp, info);
    }

    return FALSE;
}

static BOOL fort_callout_classify_blocked_log(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const FWPS_FILTER0 *filter,
        FWPS_CLASSIFY_OUT0 *classifyOut, int flagsField, int localIpField, int remoteIpField,
        int localPortField, int remotePortField, int ipProtoField, BOOL isIPv6, BOOL inbound,
        UINT32 classify_flags, const UINT32 *remote_ip, FORT_CONF_FLAGS conf_flags,
        UINT32 process_id, PCUNICODE_STRING path, PCUNICODE_STRING real_path,
        PFORT_CONF_REF conf_ref, INT8 *block_reason, BOOL blocked, PIRP *irp, ULONG_PTR *info)
{
    FORT_APP_FLAGS app_flags =
            fort_conf_app_find(&conf_ref->conf, path->Buffer, path->Length, fort_conf_exe_find);

    if (!blocked /* collect traffic, when Filter Disabled */
            || (app_flags.v == 0 && conf_flags.allow_all_new) /* collect new Blocked Programs */
            || !fort_conf_app_blocked(&conf_ref->conf, app_flags, block_reason)) {
        if (conf_flags.log_stat
                && fort_callout_classify_blocked_log_stat(inFixedValues, inMetaValues, filter,
                        classifyOut, flagsField, localIpField, remoteIpField, localPortField,
                        remotePortField, ipProtoField, isIPv6, inbound, classify_flags, remote_ip,
                        conf_flags, process_id, real_path, conf_ref, block_reason, blocked,
                        app_flags, irp, info))
            return TRUE; /* blocked */

        blocked = FALSE; /* allow */
    }

    if (app_flags.v == 0 && (conf_flags.allow_all_new || conf_flags.log_blocked)
            && conf_flags.filter_enabled) {
        app_flags.blocked = (UCHAR) blocked;
        app_flags.alerted = 1;
        app_flags.is_new = 1;

        if (NT_SUCCESS(
                    fort_conf_ref_exe_add_path(conf_ref, path->Buffer, path->Length, app_flags))) {
            fort_buffer_blocked_write(&fort_device()->buffer, blocked, process_id,
                    real_path->Length, real_path->Buffer, irp, info);
        }
    }

    return blocked;
}

static BOOL fort_callout_classify_blocked(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const FWPS_FILTER0 *filter,
        FWPS_CLASSIFY_OUT0 *classifyOut, int flagsField, int localIpField, int remoteIpField,
        int localPortField, int remotePortField, int ipProtoField, BOOL isIPv6, BOOL inbound,
        UINT32 classify_flags, const UINT32 *remote_ip, FORT_CONF_FLAGS conf_flags,
        UINT32 process_id, PUNICODE_STRING path, PUNICODE_STRING real_path, PFORT_CONF_REF conf_ref,
        INT8 *block_reason, PIRP *irp, ULONG_PTR *info)
{
    BOOL blocked = TRUE;

    if (conf_flags.filter_enabled) {
        if (conf_flags.stop_traffic)
            return TRUE; /* block all */

        if (!fort_conf_ip_is_inet(&conf_ref->conf,
                    (fort_conf_zones_ip_included_func *) fort_conf_zones_ip_included,
                    &fort_device()->conf, remote_ip, isIPv6))
            return FALSE; /* allow LocalNetwork */

        if (conf_flags.stop_inet_traffic)
            return TRUE; /* block Internet */

        if (!fort_conf_ip_inet_included(&conf_ref->conf,
                    (fort_conf_zones_ip_included_func *) fort_conf_zones_ip_included,
                    &fort_device()->conf, remote_ip, isIPv6)) {
            *block_reason = FORT_BLOCK_REASON_IP_INET;
            return TRUE; /* block address */
        }
    } else {
        if (!(conf_flags.log_stat && conf_flags.log_stat_no_filter))
            return FALSE; /* allow (Filter Disabled) */

        blocked = FALSE;
    }

    return fort_callout_classify_blocked_log(inFixedValues, inMetaValues, filter, classifyOut,
            flagsField, localIpField, remoteIpField, localPortField, remotePortField, ipProtoField,
            isIPv6, inbound, classify_flags, remote_ip, conf_flags, process_id, path, real_path,
            conf_ref, block_reason, blocked, irp, info);
}

static void fort_callout_classify_check(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const FWPS_FILTER0 *filter,
        FWPS_CLASSIFY_OUT0 *classifyOut, int flagsField, int localIpField, int remoteIpField,
        int localPortField, int remotePortField, int ipProtoField, BOOL isIPv6, BOOL inbound,
        UINT32 classify_flags, const UINT32 *remote_ip, PFORT_CONF_REF conf_ref, PIRP *irp,
        ULONG_PTR *info)
{
    const FORT_CONF_FLAGS conf_flags = conf_ref->conf.flags;

    const UINT32 process_id = (UINT32) inMetaValues->processId;

    UNICODE_STRING real_path;
    real_path.Length =
            (UINT16) (inMetaValues->processPath->size - sizeof(WCHAR)); /* chop terminating zero */
    real_path.MaximumLength = real_path.Length;
    real_path.Buffer = (PWSTR) inMetaValues->processPath->data;

    BOOL inherited = FALSE;
    UNICODE_STRING path;
    if (!fort_pstree_get_proc_name(&fort_device()->ps_tree, process_id, &path, &inherited)) {
        path = real_path;
    } else if (!inherited) {
        real_path = path;
    }

    INT8 block_reason = FORT_BLOCK_REASON_UNKNOWN;
    const BOOL blocked = fort_callout_classify_blocked(inFixedValues, inMetaValues, filter,
            classifyOut, flagsField, localIpField, remoteIpField, localPortField, remotePortField,
            ipProtoField, isIPv6, inbound, classify_flags, remote_ip, conf_flags, process_id, &path,
            &real_path, conf_ref, &block_reason, irp, info);

    if (blocked) {
        /* Log the blocked connection */
        if (block_reason != FORT_BLOCK_REASON_UNKNOWN && conf_flags.log_blocked_ip) {
            const UINT32 *local_ip = isIPv6
                    ? (const UINT32 *) inFixedValues->incomingValue[localIpField].value.byteArray16
                    : &inFixedValues->incomingValue[localIpField].value.uint32;

            const UINT16 local_port = inFixedValues->incomingValue[localPortField].value.uint16;
            const UINT16 remote_port = inFixedValues->incomingValue[remotePortField].value.uint16;
            const IPPROTO ip_proto =
                    (IPPROTO) inFixedValues->incomingValue[ipProtoField].value.uint8;

            fort_buffer_blocked_ip_write(&fort_device()->buffer, isIPv6, inbound, inherited,
                    block_reason, ip_proto, local_port, remote_port, local_ip, remote_ip,
                    process_id, real_path.Length, real_path.Buffer, irp, info);
        }

        /* Block the connection */
        fort_callout_classify_block(classifyOut);
    } else {
        if (block_reason == FORT_BLOCK_REASON_NONE) {
            /* Continue the search */
            fort_callout_classify_continue(classifyOut);
        } else {
            /* Allow the connection */
            fort_callout_classify_permit(filter, classifyOut);
        }
    }
}

static void fort_callout_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const FWPS_FILTER0 *filter,
        FWPS_CLASSIFY_OUT0 *classifyOut, int flagsField, int localIpField, int remoteIpField,
        int localPortField, int remotePortField, int ipProtoField, BOOL isIPv6, BOOL inbound)
{
    PIRP irp = NULL;
    ULONG_PTR info;

    const UINT32 classify_flags = inFixedValues->incomingValue[flagsField].value.uint32;

    const UINT32 *remote_ip = isIPv6
            ? (const UINT32 *) inFixedValues->incomingValue[remoteIpField].value.byteArray16
            : &inFixedValues->incomingValue[remoteIpField].value.uint32;

    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;

    if (!device_conf->conf_flags.filter_locals
            && ((classify_flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
                    || fort_addr_is_local_broadcast(remote_ip, isIPv6))) {
        fort_callout_classify_permit(filter, classifyOut);
        return;
    }

    PFORT_CONF_REF conf_ref = fort_conf_ref_take(device_conf);

    if (conf_ref == NULL) {
        if (fort_device_flag(device_conf, FORT_DEVICE_PROV_BOOT) != 0) {
            fort_callout_classify_block(classifyOut);
        } else {
            fort_callout_classify_continue(classifyOut);
        }
        return;
    }

    fort_callout_classify_check(inFixedValues, inMetaValues, filter, classifyOut, flagsField,
            localIpField, remoteIpField, localPortField, remotePortField, ipProtoField, isIPv6,
            inbound, classify_flags, remote_ip, conf_ref, &irp, &info);

    fort_conf_ref_put(device_conf, conf_ref);

    if (irp != NULL) {
        fort_request_complete_info(irp, STATUS_SUCCESS, info);
    }
}

static void NTAPI fort_callout_connect_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, void *layerData,
        const FWPS_FILTER0 *filter, const UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    UNUSED(layerData);
    UNUSED(flowContext);

    fort_callout_classify(inFixedValues, inMetaValues, filter, classifyOut,
            FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS, FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS,
            FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS,
            FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT,
            FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT,
            FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL, /*isIPv6=*/FALSE, /*inbound=*/FALSE);
}

static void NTAPI fort_callout_connect_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, void *layerData,
        const FWPS_FILTER0 *filter, const UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    UNUSED(layerData);
    UNUSED(flowContext);

    fort_callout_classify(inFixedValues, inMetaValues, filter, classifyOut,
            FWPS_FIELD_ALE_AUTH_CONNECT_V6_FLAGS, FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_ADDRESS,
            FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS,
            FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_PORT,
            FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT,
            FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_PROTOCOL, /*isIPv6=*/TRUE, /*inbound=*/FALSE);
}

static void NTAPI fort_callout_accept_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, void *layerData,
        const FWPS_FILTER0 *filter, const UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    UNUSED(layerData);
    UNUSED(flowContext);

    fort_callout_classify(inFixedValues, inMetaValues, filter, classifyOut,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL, /*isIPv6=*/FALSE, /*inbound=*/TRUE);
}

static void NTAPI fort_callout_accept_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, void *layerData,
        const FWPS_FILTER0 *filter, const UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    UNUSED(layerData);
    UNUSED(flowContext);

    fort_callout_classify(inFixedValues, inMetaValues, filter, classifyOut,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_FLAGS,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_ADDRESS,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_PORT,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_PORT,
            FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_PROTOCOL, /*isIPv6=*/TRUE, /*inbound=*/TRUE);
}

static NTSTATUS NTAPI fort_callout_notify(
        FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID *filterKey, const FWPS_FILTER0 *filter)
{
    UNUSED(notifyType);
    UNUSED(filterKey);
    UNUSED(filter);

    return STATUS_SUCCESS;
}

static void fort_callout_flow_classify(const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
        UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut, UINT32 dataSize, BOOL inbound)
{
    const UINT32 headerSize = inbound ? inMetaValues->transportHeaderSize : 0;

    UNUSED(classifyOut);

    fort_flow_classify(&fort_device()->stat, flowContext, headerSize + dataSize, inbound);
}

static void NTAPI fort_callout_stream_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, FWPS_STREAM_CALLOUT_IO_PACKET0 *packet,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    const FWPS_STREAM_DATA0 *streamData = packet->streamData;
    const UINT32 streamFlags = streamData->flags;
    const UINT32 dataSize = (UINT32) streamData->dataLength;

    const BOOL inbound = (streamFlags & FWPS_STREAM_FLAG_RECEIVE) != 0;

    UNUSED(inFixedValues);

    fort_callout_flow_classify(inMetaValues, flowContext, classifyOut, dataSize, inbound);

    fort_callout_classify_permit(filter, classifyOut);
    return;
}

static void fort_callout_datagram_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        int directionField)
{
    const PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList);
    const UINT32 dataSize = NET_BUFFER_DATA_LENGTH(netBuf);

    const FWP_DIRECTION direction =
            (FWP_DIRECTION) inFixedValues->incomingValue[directionField].value.uint8;
    const BOOL inbound = (direction == FWP_DIRECTION_INBOUND);

    fort_callout_flow_classify(inMetaValues, flowContext, classifyOut, dataSize, inbound);

    fort_callout_classify_permit(filter, classifyOut);
}

static void NTAPI fort_callout_datagram_classify_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_datagram_classify(inFixedValues, inMetaValues, netBufList, filter, flowContext,
            classifyOut, FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION);
}

static void NTAPI fort_callout_datagram_classify_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_datagram_classify(inFixedValues, inMetaValues, netBufList, filter, flowContext,
            classifyOut, FWPS_FIELD_DATAGRAM_DATA_V6_DIRECTION);
}

static void NTAPI fort_callout_flow_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);

    fort_flow_delete(&fort_device()->stat, flowContext);
}

static BOOL fort_callout_mac_frame_classify_packet(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);

    const BOOL inbound = (flow_flags & FORT_FLOW_INBOUND) != 0;
    const UCHAR speed_limit = inbound ? FORT_FLOW_SPEED_LIMIT_IN : FORT_FLOW_SPEED_LIMIT_OUT;

    if ((flow_flags & speed_limit) == 0
            || fort_device_flag(&fort_device()->conf, FORT_DEVICE_POWER_OFF) != 0)
        return FALSE;

    return fort_shaper_packet_process(&fort_device()->shaper, inFixedValues, inMetaValues,
            netBufList, inbound, flow->opt.group_index);
}

static void NTAPI fort_callout_mac_frame_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        const void *classifyContext, const FWPS_FILTER0 *filter, UINT64 flowContext,
        FWPS_CLASSIFY_OUT0 *classifyOut)
{
    if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
        return; /* Can't act on the packet */

    if (!FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED)
            && netBufList != NULL) {
        if (fort_callout_mac_frame_classify_packet(
                    inFixedValues, inMetaValues, netBufList, flowContext, classifyOut)) {
            fort_callout_classify_drop(classifyOut); /* drop */
            return;
        }
    }

    fort_callout_classify_permit(filter, classifyOut); /* permit */
}

static void NTAPI fort_callout_mac_frame_delete(
        UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);
    UNUSED(flowContext);
}

FORT_API NTSTATUS fort_callout_install(PDEVICE_OBJECT device)
{
    PFORT_STAT stat = &fort_device()->stat;

    FWPS_CALLOUT0 c;
    NTSTATUS status;

    RtlZeroMemory(&c, sizeof(FWPS_CALLOUT0));

    c.notifyFn = (FWPS_CALLOUT_NOTIFY_FN0) fort_callout_notify;

    /* IPv4 connect callout */
    c.calloutKey = FORT_GUID_CALLOUT_CONNECT_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_connect_v4;

    status = FwpsCalloutRegister0(device, &c, &fort_device()->connect4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Connect V4: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_CONNECT_V4_ERROR, status, 0, 0);
        return status;
    }

    /* IPv6 connect callout */
    c.calloutKey = FORT_GUID_CALLOUT_CONNECT_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_connect_v6;

    status = FwpsCalloutRegister0(device, &c, &fort_device()->connect6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Connect V6: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_CONNECT_V6_ERROR, status, 0, 0);
        return status;
    }

    /* IPv4 accept callout */
    c.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_accept_v4;

    status = FwpsCalloutRegister0(device, &c, &fort_device()->accept4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Accept V4: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_ACCEPT_V4_ERROR, status, 0, 0);
        return status;
    }

    /* IPv6 accept callout */
    c.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_accept_v6;

    status = FwpsCalloutRegister0(device, &c, &fort_device()->accept6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Accept V6: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_ACCEPT_V6_ERROR, status, 0, 0);
        return status;
    }

    /* IPv4 stream callout */
    c.calloutKey = FORT_GUID_CALLOUT_STREAM_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_stream_classify;

    c.flowDeleteFn = fort_callout_flow_delete;
    c.flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW;

    status = FwpsCalloutRegister0(device, &c, &stat->stream4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Stream V4: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_STREAM_V4_ERROR, status, 0, 0);
        return status;
    }

    /* IPv6 stream callout */
    c.calloutKey = FORT_GUID_CALLOUT_STREAM_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_stream_classify;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->stream6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Stream V6: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_STREAM_V6_ERROR, status, 0, 0);
        return status;
    }

    /* IPv4 datagram callout */
    c.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_datagram_classify_v4;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->datagram4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Datagram V4: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_DATAGRAM_V4_ERROR, status, 0, 0);
        return status;
    }

    /* IPv6 datagram callout */
    c.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_datagram_classify_v6;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->datagram6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Datagram V6: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_DATAGRAM_V6_ERROR, status, 0, 0);
        return status;
    }

    /* Inbound MAC Frame callout */
    c.calloutKey = FORT_GUID_CALLOUT_IN_MAC_FRAME;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_mac_frame_classify;

    c.flowDeleteFn = fort_callout_mac_frame_delete;
    /* reuse c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->in_mac_frame_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Inbound MAC Frame: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_INBOUND_MAC_FRAME_ERROR, status, 0, 0);
        return status;
    }

    /* Outbound MAC Frame callout */
    c.calloutKey = FORT_GUID_CALLOUT_OUT_MAC_FRAME;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_mac_frame_classify;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->out_mac_frame_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Outbound MAC Frame: Error: %x\n", status);
        TRACE(FORT_CALLOUT_REGISTER_OUTBOUND_MAC_FRAME_ERROR, status, 0, 0);
        return status;
    }

    return STATUS_SUCCESS;
}

FORT_API void fort_callout_remove(void)
{
    PFORT_STAT stat = &fort_device()->stat;

    if (fort_device()->connect4_id) {
        FwpsCalloutUnregisterById0(fort_device()->connect4_id);
        fort_device()->connect4_id = 0;
    }

    if (fort_device()->connect6_id) {
        FwpsCalloutUnregisterById0(fort_device()->connect6_id);
        fort_device()->connect6_id = 0;
    }

    if (fort_device()->accept4_id) {
        FwpsCalloutUnregisterById0(fort_device()->accept4_id);
        fort_device()->accept4_id = 0;
    }

    if (fort_device()->accept6_id) {
        FwpsCalloutUnregisterById0(fort_device()->accept6_id);
        fort_device()->accept6_id = 0;
    }

    if (stat->stream4_id) {
        FwpsCalloutUnregisterById0(stat->stream4_id);
        stat->stream4_id = 0;
    }

    if (stat->stream6_id) {
        FwpsCalloutUnregisterById0(stat->stream6_id);
        stat->stream6_id = 0;
    }

    if (stat->datagram4_id) {
        FwpsCalloutUnregisterById0(stat->datagram4_id);
        stat->datagram4_id = 0;
    }

    if (stat->datagram6_id) {
        FwpsCalloutUnregisterById0(stat->datagram6_id);
        stat->datagram6_id = 0;
    }

    if (stat->in_mac_frame_id) {
        FwpsCalloutUnregisterById0(stat->in_mac_frame_id);
        stat->in_mac_frame_id = 0;
    }

    if (stat->out_mac_frame_id) {
        FwpsCalloutUnregisterById0(stat->out_mac_frame_id);
        stat->out_mac_frame_id = 0;
    }
}

static NTSTATUS fort_callout_force_reauth_prov(
        const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags, HANDLE engine)
{
    NTSTATUS status;

    /* Check provider filters */
    BOOL prov_recreated = FALSE;
    if (old_conf_flags.prov_boot != conf_flags.prov_boot) {
        fort_prov_unregister(engine);

        if ((status = fort_prov_register(engine, conf_flags.prov_boot)))
            return status;

        prov_recreated = TRUE;
    }

    /* Check flow filter */
    {
        const PFORT_CONF_GROUP conf_group = &fort_device()->stat.conf_group;
        const UINT16 limit_bits = conf_group->limit_bits;

        const BOOL old_filter_packets =
                fort_device_flag(&fort_device()->conf, FORT_DEVICE_FILTER_PACKETS) != 0;
        const BOOL filter_packets = (conf_flags.group_bits & limit_bits) != 0;

        if (prov_recreated || old_conf_flags.log_stat != conf_flags.log_stat
                || old_filter_packets != filter_packets) {
            fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_FILTER_PACKETS, filter_packets);

            fort_prov_flow_unregister(engine);

            if (conf_flags.log_stat) {
                if ((status = fort_prov_flow_register(engine, filter_packets)))
                    return status;
            }
        }
    }

    /* Force reauth filter */
    return fort_prov_reauth(engine);
}

FORT_API NTSTATUS fort_callout_force_reauth(const FORT_CONF_FLAGS old_conf_flags)
{
    NTSTATUS status;

    fort_timer_update(&fort_device()->log_timer, /*run=*/FALSE);

    /* Check app group periods & update group_bits */
    {
        int periods_n = 0;

        fort_conf_ref_period_update(&fort_device()->conf, /*force=*/TRUE, &periods_n);

        fort_timer_update(&fort_device()->app_timer, /*run=*/(periods_n != 0));
    }

    const FORT_CONF_FLAGS conf_flags = fort_device()->conf.conf_flags;

    /* Handle log_stat */
    fort_stat_log_update(&fort_device()->stat, conf_flags.log_stat);

    /* Open provider */
    HANDLE engine;
    status = fort_prov_open(&engine);
    if (NT_SUCCESS(status)) {
        fort_prov_trans_begin(engine);

        status = fort_callout_force_reauth_prov(old_conf_flags, conf_flags, engine);

        status = fort_prov_trans_close(engine, status);
    }

    if (NT_SUCCESS(status)) {
        const BOOL log_enabled = (conf_flags.allow_all_new || conf_flags.log_blocked
                || conf_flags.log_stat || conf_flags.log_blocked_ip);

        fort_timer_update(&fort_device()->log_timer, /*run=*/log_enabled);
    } else {
        LOG("Callout Reauth: Error: %x\n", status);
        TRACE(FORT_CALLOUT_CALLOUT_REAUTH_ERROR, status, 0, 0);
    }

    return status;
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
    {
        LARGE_INTEGER system_time;
        PCHAR out;

        KeQuerySystemTime(&system_time);

        if (stat->system_time.QuadPart != system_time.QuadPart
                && NT_SUCCESS(fort_buffer_prepare(buf, FORT_LOG_TIME_SIZE, &out, &irp, &info))) {
            const INT64 unix_time = fort_system_to_unix_time(system_time.QuadPart);

            const UCHAR old_stat_flags = fort_stat_flags_set(stat, FORT_STAT_TIME_CHANGED, FALSE);
            const BOOL time_changed = ((old_stat_flags & FORT_STAT_TIME_CHANGED) != 0);

            stat->system_time = system_time;

            fort_log_time_write(out, time_changed, unix_time);
        }
    }

    /* Flush traffic statistics */
    while (stat->proc_active_count != 0) {
        const UINT16 proc_count = (stat->proc_active_count < FORT_LOG_STAT_BUFFER_PROC_COUNT)
                ? stat->proc_active_count
                : FORT_LOG_STAT_BUFFER_PROC_COUNT;
        const UINT32 len = FORT_LOG_STAT_SIZE(proc_count);
        PCHAR out;
        NTSTATUS status;

        status = fort_buffer_prepare(buf, len, &out, &irp, &info);
        if (!NT_SUCCESS(status)) {
            LOG("Callout Timer: Error: %x\n", status);
            TRACE(FORT_CALLOUT_CALLOUT_TIMER_ERROR, status, 0, 0);
            break;
        }

        fort_log_stat_traf_header_write(out, proc_count);
        out += FORT_LOG_STAT_HEADER_SIZE;

        fort_stat_dpc_traf_flush(stat, proc_count, out);
    }

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
