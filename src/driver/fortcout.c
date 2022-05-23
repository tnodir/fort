/* Fort Firewall Callouts */

#include "fortcout.h"

#include "common/fortdef.h"
#include "common/fortioctl.h"
#include "common/fortprov.h"

#include "fortdev.h"
#include "fortps.h"
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
            group_index, isIPv6, is_tcp, is_reauth, &is_new_proc);

    if (!NT_SUCCESS(status)) {
        if (status == FORT_STATUS_FLOW_BLOCK) {
            *block_reason = FORT_BLOCK_REASON_REAUTH;
            return TRUE; /* block (Reauth) */
        }

        LOG("Classify v4: Flow assoc. error: %x\n", status);
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
    if (!fort_pstree_get_proc_name(
                &fort_device()->ps_tree, conf_ref, process_id, &path, &inherited)) {
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
        if (fort_device_flag(device_conf, FORT_DEVICE_PROV_BOOT)) {
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

static void NTAPI fort_packet_inject_complete(
        PFORT_PACKET pkt, PNET_BUFFER_LIST clonedNetBufList, BOOLEAN dispatchLevel)
{
    fort_defer_packet_free(&fort_device()->defer, pkt, clonedNetBufList, dispatchLevel);
}

static void fort_callout_defer_packet_flush(UINT32 list_bits, BOOL dispatchLevel)
{
    fort_defer_packet_flush(
            &fort_device()->defer, fort_packet_inject_complete, list_bits, dispatchLevel);
}

static void fort_callout_defer_stream_flush(UINT64 flow_id, BOOL dispatchLevel)
{
    UNUSED(dispatchLevel);

    fort_defer_stream_flush(&fort_device()->defer, fort_packet_inject_complete, flow_id, FALSE);
}

FORT_API void fort_callout_defer_flush(void)
{
    fort_callout_defer_packet_flush(FORT_DEFER_FLUSH_ALL, FALSE);
    fort_callout_defer_stream_flush(FORT_DEFER_STREAM_ALL, FALSE);
}

static void fort_callout_flow_classify(const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
        UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut, UINT32 dataSize, BOOL isIPv6,
        BOOL is_tcp, BOOL inbound)
{
    const UINT32 headerSize = inbound ? inMetaValues->transportHeaderSize : 0;

    UNUSED(classifyOut);

    fort_flow_classify(
            &fort_device()->stat, flowContext, headerSize + dataSize, isIPv6, is_tcp, inbound);
}

static BOOL fort_callout_stream_classify_fragment(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, FWPS_STREAM_CALLOUT_IO_PACKET0 *packet,
        const FWPS_FILTER0 *filter, UINT64 flowContext, BOOL isIPv6)
{
    const FWPS_STREAM_DATA0 *streamData = packet->streamData;
    const UINT32 streamFlags = streamData->flags;

    if ((streamFlags
                & (FWPS_STREAM_FLAG_SEND | FWPS_STREAM_FLAG_SEND_EXPEDITED
                        | FWPS_STREAM_FLAG_SEND_DISCONNECT))
            != FWPS_STREAM_FLAG_SEND)
        return FALSE;

    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);

    const UCHAR fragment_flags =
            (flow_flags & (FORT_FLOW_FRAGMENT | FORT_FLOW_FRAGMENT_DEFER | FORT_FLOW_FRAGMENTED));

    if (fragment_flags != 0 && !(fragment_flags & FORT_FLOW_FRAGMENTED)) {
        const UINT32 dataSize = (UINT32) streamData->dataLength;
        const BOOL inbound = (streamFlags & FWPS_STREAM_FLAG_RECEIVE) != 0;
        const UCHAR fragment_size = 3;

        if (fragment_flags & FORT_FLOW_FRAGMENT_DEFER) {
            const NTSTATUS status = fort_defer_stream_add(&fort_device()->defer, inFixedValues,
                    inMetaValues, streamData, filter, isIPv6, inbound);

            if (NT_SUCCESS(status))
                return TRUE;

            fort_flow_flags_set(flow, FORT_FLOW_FRAGMENTED, TRUE);
        } else if (dataSize > fragment_size) {
            packet->countBytesEnforced = fragment_size;

            fort_flow_flags_set(flow, FORT_FLOW_FRAGMENT_DEFER, TRUE);
        }
    }

    return FALSE;
}

static void fort_callout_stream_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, FWPS_STREAM_CALLOUT_IO_PACKET0 *packet,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        BOOL isIPv6)
{
    const FWPS_STREAM_DATA0 *streamData = packet->streamData;
    const UINT32 streamFlags = streamData->flags;
    const UINT32 dataSize = (UINT32) streamData->dataLength;

    const BOOL inbound = (streamFlags & FWPS_STREAM_FLAG_RECEIVE) != 0;

    UNUSED(inFixedValues);

    fort_callout_flow_classify(inMetaValues, flowContext, classifyOut, dataSize, isIPv6,
            /*is_tcp=*/TRUE, inbound);

/* Flush flow's deferred TCP packets on FIN */
#if 0
    if (streamFlags & (FWPS_STREAM_FLAG_RECEIVE_DISCONNECT | FWPS_STREAM_FLAG_SEND_DISCONNECT)) {
        PFORT_FLOW flow = (PFORT_FLOW) flowContext;

        const UCHAR flow_flags = fort_flow_flags(flow);

        if (flow_flags & FORT_FLOW_SPEED_LIMIT) {
            fort_callout_defer_packet_flush(flow->flow_id, FORT_DEFER_FLUSH_ALL, FALSE);
        }

        if (flow_flags & FORT_FLOW_FRAGMENT) {
            fort_callout_defer_stream_flush(flow->flow_id, FALSE);
        }

        goto permit;
    }
#endif

    /* Fragment first TCP packet */
    if (fort_callout_stream_classify_fragment(
                inFixedValues, inMetaValues, packet, filter, flowContext, isIPv6))
        goto drop;

    /* permit: */
    fort_callout_classify_permit(filter, classifyOut);
    return;

drop:
    fort_callout_classify_drop(classifyOut);
    return;
}

static void NTAPI fort_callout_stream_classify_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, FWPS_STREAM_CALLOUT_IO_PACKET0 *packet,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_stream_classify(inFixedValues, inMetaValues, packet, filter, flowContext,
            classifyOut, /*isIPv6=*/FALSE);
}

static void NTAPI fort_callout_stream_classify_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, FWPS_STREAM_CALLOUT_IO_PACKET0 *packet,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_stream_classify(
            inFixedValues, inMetaValues, packet, filter, flowContext, classifyOut, /*isIPv6=*/TRUE);
}

static void fort_callout_datagram_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        int directionField, BOOL isIPv6)
{
    const PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList);
    const UINT32 dataSize = NET_BUFFER_DATA_LENGTH(netBuf);

    const FWP_DIRECTION direction =
            (FWP_DIRECTION) inFixedValues->incomingValue[directionField].value.uint8;
    const BOOL inbound = (direction == FWP_DIRECTION_INBOUND);

    fort_callout_flow_classify(inMetaValues, flowContext, classifyOut, dataSize, isIPv6,
            /*is_tcp=*/FALSE, inbound);

    fort_callout_classify_permit(filter, classifyOut);
}

static void NTAPI fort_callout_datagram_classify_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_datagram_classify(inFixedValues, inMetaValues, netBufList, filter, flowContext,
            classifyOut, FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION, /*isIPv6=*/FALSE);
}

static void NTAPI fort_callout_datagram_classify_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_datagram_classify(inFixedValues, inMetaValues, netBufList, filter, flowContext,
            classifyOut, FWPS_FIELD_DATAGRAM_DATA_V6_DIRECTION, /*isIPv6=*/TRUE);
}

static void NTAPI fort_callout_flow_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);

    fort_flow_delete(&fort_device()->stat, flowContext);
}

static BOOL fort_callout_transport_classify_packet(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        PNET_BUFFER netBuf, BOOL isIPv6, BOOL inbound)
{
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);

    const UCHAR defer_flag = inbound ? FORT_FLOW_DEFER_IN : FORT_FLOW_DEFER_OUT;
    const UCHAR ack_speed_limit = inbound ? FORT_FLOW_SPEED_LIMIT_OUT : FORT_FLOW_SPEED_LIMIT_IN;

    const UCHAR speed_defer_flags = ack_speed_limit | defer_flag;
    const BOOL defer_flow = (flow_flags & speed_defer_flags) == speed_defer_flags
            && !fort_device_flag(&fort_device()->conf, FORT_DEVICE_POWER_OFF);

    if (isIPv6) /* TODO: Support IPv6 for speed limits */
        return FALSE;

    /* Position in the packet data:
     * FWPS_LAYER_INBOUND_TRANSPORT_V4: The beginning of the data.
     * FWPS_LAYER_OUTBOUND_TRANSPORT_V4: The beginning of the transport header.
     */
    const UINT32 headerOffset = inbound ? 0 : sizeof(TCP_HEADER);

    /* Defer TCP Pure (zero length) ACK-packets */
    if (defer_flow && NET_BUFFER_DATA_LENGTH(netBuf) == headerOffset) {
        const NTSTATUS status = fort_defer_packet_add(&fort_device()->defer, inFixedValues,
                inMetaValues, netBufList, isIPv6, inbound, flow->opt.group_index);

        if (NT_SUCCESS(status))
            return TRUE;

        if (status == STATUS_CANT_TERMINATE_SELF) {
            /* Clear ACK deferring */
            fort_flow_flags_set(flow, defer_flag, FALSE);
        }

        return FALSE;
    }

    /* Fragment first TCP packet */
    const BOOL fragment_packet = !inbound
            && (flow_flags & (FORT_FLOW_FRAGMENT_DEFER | FORT_FLOW_FRAGMENTED))
                    == FORT_FLOW_FRAGMENT_DEFER;

    if (fragment_packet) {
        fort_defer_stream_flush(
                &fort_device()->defer, fort_packet_inject_complete, flow->flow_id, FALSE);

        fort_flow_flags_set(flow, FORT_FLOW_FRAGMENTED, TRUE);
    }

    return FALSE;
}

static void fort_callout_transport_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        BOOL isIPv6, BOOL inbound)
{
    PNET_BUFFER netBuf;

    if (!FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED)
            && netBufList != NULL && (netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList)) != NULL) {

        if (fort_callout_transport_classify_packet(inFixedValues, inMetaValues, netBufList, filter,
                    flowContext, classifyOut, netBuf, isIPv6, inbound)) {
            fort_callout_classify_drop(classifyOut); /* drop */
            return;
        }
    }

    fort_callout_classify_permit(filter, classifyOut); /* permit */
}

static void NTAPI fort_callout_in_transport_classify_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_transport_classify(inFixedValues, inMetaValues, netBufList, filter, flowContext,
            classifyOut, /*isIPv6=*/FALSE, /*inbound=*/TRUE);
}
static void NTAPI fort_callout_in_transport_classify_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_transport_classify(inFixedValues, inMetaValues, netBufList, filter, flowContext,
            classifyOut, /*isIPv6=*/TRUE, /*inbound=*/TRUE);
}

static void NTAPI fort_callout_out_transport_classify_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_transport_classify(inFixedValues, inMetaValues, netBufList, filter, flowContext,
            classifyOut, /*isIPv6=*/FALSE, /*inbound=*/FALSE);
}

static void NTAPI fort_callout_out_transport_classify_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, const PNET_BUFFER_LIST netBufList,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_transport_classify(inFixedValues, inMetaValues, netBufList, filter, flowContext,
            classifyOut, /*isIPv6=*/TRUE, /*inbound=*/FALSE);
}

static void NTAPI fort_callout_transport_delete(
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
        return status;
    }

    /* IPv6 connect callout */
    c.calloutKey = FORT_GUID_CALLOUT_CONNECT_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_connect_v6;

    status = FwpsCalloutRegister0(device, &c, &fort_device()->connect6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Connect V6: Error: %x\n", status);
        return status;
    }

    /* IPv4 accept callout */
    c.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_accept_v4;

    status = FwpsCalloutRegister0(device, &c, &fort_device()->accept4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Accept V4: Error: %x\n", status);
        return status;
    }

    /* IPv6 accept callout */
    c.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_accept_v6;

    status = FwpsCalloutRegister0(device, &c, &fort_device()->accept6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Accept V6: Error: %x\n", status);
        return status;
    }

    /* IPv4 stream callout */
    c.calloutKey = FORT_GUID_CALLOUT_STREAM_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_stream_classify_v4;

    c.flowDeleteFn = fort_callout_flow_delete;
    c.flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW;

    status = FwpsCalloutRegister0(device, &c, &stat->stream4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Stream V4: Error: %x\n", status);
        return status;
    }

    /* IPv6 stream callout */
    c.calloutKey = FORT_GUID_CALLOUT_STREAM_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_stream_classify_v6;

    c.flowDeleteFn = fort_callout_flow_delete;
    c.flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW;

    status = FwpsCalloutRegister0(device, &c, &stat->stream6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Stream V6: Error: %x\n", status);
        return status;
    }

    /* IPv4 datagram callout */
    c.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_datagram_classify_v4;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->datagram4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Datagram V4: Error: %x\n", status);
        return status;
    }

    /* IPv6 datagram callout */
    c.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_datagram_classify_v6;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->datagram6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Datagram V6: Error: %x\n", status);
        return status;
    }

    /* IPv4 inbound transport callout */
    c.calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_in_transport_classify_v4;

    c.flowDeleteFn = fort_callout_transport_delete;
    /* reuse c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->in_transport4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Inbound Transport V4: Error: %x\n", status);
        return status;
    }

    /* IPv6 inbound transport callout */
    c.calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_in_transport_classify_v6;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->in_transport6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Inbound Transport V6: Error: %x\n", status);
        return status;
    }

    /* IPv4 outbound transport callout */
    c.calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V4;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_out_transport_classify_v4;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->out_transport4_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Outbound Transport V4: Error: %x\n", status);
        return status;
    }

    /* IPv6 outbound transport callout */
    c.calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V6;
    c.classifyFn = (FWPS_CALLOUT_CLASSIFY_FN0) fort_callout_out_transport_classify_v6;

    /* reuse c.flowDeleteFn & c.flags */

    status = FwpsCalloutRegister0(device, &c, &stat->out_transport6_id);
    if (!NT_SUCCESS(status)) {
        LOG("Register Outbound Transport V6: Error: %x\n", status);
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

    if (stat->in_transport4_id) {
        FwpsCalloutUnregisterById0(stat->in_transport4_id);
        stat->in_transport4_id = 0;
    }

    if (stat->in_transport6_id) {
        FwpsCalloutUnregisterById0(stat->in_transport6_id);
        stat->in_transport6_id = 0;
    }

    if (stat->out_transport4_id) {
        FwpsCalloutUnregisterById0(stat->out_transport4_id);
        stat->out_transport4_id = 0;
    }

    if (stat->out_transport6_id) {
        FwpsCalloutUnregisterById0(stat->out_transport6_id);
        stat->out_transport6_id = 0;
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
        PFORT_STAT stat = &fort_device()->stat;

        const PFORT_CONF_GROUP conf_group = &stat->conf_group;
        const UINT16 filter_bits = (conf_group->fragment_bits | conf_group->limit_bits);

        const BOOL old_filter_transport =
                fort_device_flag(&fort_device()->conf, FORT_DEVICE_FILTER_TRANSPORT) != 0;
        const BOOL filter_transport = (conf_flags.group_bits & filter_bits) != 0;

        if (prov_recreated || old_conf_flags.log_stat != conf_flags.log_stat
                || old_filter_transport != filter_transport) {
            fort_device_flag_set(
                    &fort_device()->conf, FORT_DEVICE_FILTER_TRANSPORT, filter_transport);

            fort_prov_flow_unregister(engine);

            if (conf_flags.log_stat) {
                if ((status = fort_prov_flow_register(engine, filter_transport)))
                    return status;
            }
        }
    }

    /* Force reauth filter */
    return fort_prov_reauth(engine);
}

FORT_API NTSTATUS fort_callout_force_reauth(
        const FORT_CONF_FLAGS old_conf_flags, UINT32 defer_flush_bits)
{
    NTSTATUS status;

    fort_timer_update(&fort_device()->log_timer, FALSE);

    /* Check app group periods & update group_bits */
    {
        int periods_n = 0;

        fort_conf_ref_period_update(&fort_device()->conf, TRUE, &periods_n);

        fort_timer_update(&fort_device()->app_timer, (periods_n != 0));
    }

    const FORT_CONF_FLAGS conf_flags = fort_device()->conf.conf_flags;

    /* Handle log_stat */
    if (old_conf_flags.log_stat != conf_flags.log_stat) {
        PFORT_STAT stat = &fort_device()->stat;

        fort_stat_update(stat, conf_flags.log_stat);

        if (!conf_flags.log_stat) {
            defer_flush_bits = FORT_DEFER_FLUSH_ALL;
        }
    }

    if (defer_flush_bits != 0) {
        fort_callout_defer_packet_flush(defer_flush_bits, FALSE);
    }

    /* Open provider */
    HANDLE engine;
    status = fort_prov_open(&engine);
    if (NT_SUCCESS(status)) {
        fort_prov_trans_begin(engine);

        status = fort_callout_force_reauth_prov(old_conf_flags, conf_flags, engine);

        status = fort_prov_trans_close(engine, status);
    }

    if (NT_SUCCESS(status)) {
        fort_timer_update(&fort_device()->log_timer,
                (conf_flags.allow_all_new || conf_flags.log_blocked || conf_flags.log_stat
                        || conf_flags.log_blocked_ip));
    } else {
        LOG("Callout Reauth: Error: %x\n", status);
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

            const BOOL time_changed = ((fort_stat_flags_set(stat, FORT_STAT_TIME_CHANGED, FALSE)
                                               & FORT_STAT_TIME_CHANGED)
                    != 0);

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
            break;
        }

        fort_log_stat_traf_header_write(out, proc_count);
        out += FORT_LOG_STAT_HEADER_SIZE;

        fort_stat_dpc_traf_flush(stat, proc_count, out);
    }

    /* Flush process group statistics */
    const UINT32 defer_flush_bits = fort_stat_dpc_group_flush(stat);

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

    /* Flush deferred packets */
    fort_callout_defer_packet_flush(defer_flush_bits, TRUE);
}
