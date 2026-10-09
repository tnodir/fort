/* Fort Firewall Packets Re-injection */

#include "fortpkt.h"

#include "fortdbg.h"
#include "fortdev.h"
#include "forttrace.h"
#include "fortutl.h"

#define HTONL(l) _byteswap_ulong(l)

inline static HANDLE fort_packet_injection_id(BOOL isIPv6, BOOL inbound)
{
    PFORT_PENDING pending = &fort_device()->pending;

    return isIPv6
            ? (inbound ? pending->injection_transport6_in_id : pending->injection_transport6_out_id)
            : (inbound ? pending->injection_transport4_in_id
                       : pending->injection_transport4_out_id);
}

inline static FWPS_PACKET_INJECTION_STATE fort_packet_injection_state(PCFORT_CALLOUT_ARG ca)
{
    if (ca->netBufList == NULL)
        return FWPS_PACKET_NOT_INJECTED;

    const HANDLE injection_id = fort_packet_injection_id(ca->isIPv6, ca->inbound);

    return FwpsQueryPacketInjectionState0(injection_id, ca->netBufList, NULL);
}

FORT_API BOOL fort_packet_injected_by_self(PCFORT_CALLOUT_ARG ca)
{
    const FWPS_PACKET_INJECTION_STATE state = fort_packet_injection_state(ca);

    return (state == FWPS_PACKET_INJECTED_BY_SELF
            || state == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF);
}

FORT_API BOOL fort_packet_injected_by_other(PCFORT_CALLOUT_ARG ca)
{
    return fort_packet_injection_state(ca) == FWPS_PACKET_INJECTED_BY_OTHER;
}

static FWPS_PACKET_LIST_INBOUND_IPSEC_INFORMATION0 fort_packet_get_ipsec_inbound_info(
        PCFORT_CALLOUT_ARG ca)
{
    if (!ca->inbound || ca->netBufList == NULL) {
        const FWPS_PACKET_LIST_INBOUND_IPSEC_INFORMATION0 info = { 0 };
        return info;
    }

    FWPS_PACKET_LIST_INFORMATION0 packet_info;
    RtlZeroMemory(&packet_info, sizeof(FWPS_PACKET_LIST_INFORMATION0));

    FwpsGetPacketListSecurityInformation0(ca->netBufList,
            FWPS_PACKET_LIST_INFORMATION_QUERY_IPSEC | FWPS_PACKET_LIST_INFORMATION_QUERY_INBOUND,
            &packet_info);

    return packet_info.ipsecInformation.inbound;
}

FORT_API BOOL fort_packet_is_ipsec_protected(PCFORT_CALLOUT_ARG ca)
{
    const FWPS_PACKET_LIST_INBOUND_IPSEC_INFORMATION0 info = fort_packet_get_ipsec_inbound_info(ca);
    return info.isSecure;
}

static void fort_packet_free_cloned(PNET_BUFFER_LIST clonedNetBufList)
{
    if (clonedNetBufList == NULL)
        return;

    const NTSTATUS status = clonedNetBufList->Status;

    if (!NT_SUCCESS(status) && status != STATUS_NOT_FOUND) {
        LOG("Shaper: Packet injection error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_INJECTION_ERROR, status, 0, 0);
    }

    FwpsFreeCloneNetBufferList0(clonedNetBufList, 0);
}

inline static void fort_packet_free_out(PFORT_PACKET_OUT pkt)
{
    if (pkt->controlData != NULL) {
        fort_mem_free(pkt->controlData, FORT_PACKET_POOL_TAG);
    }
}

FORT_API void fort_packet_free(PFORT_PACKET_IO pkt)
{
    if ((pkt->flags & FORT_PACKET_INBOUND) == 0) {
        fort_packet_free_out(&pkt->out);
    }

    fort_packet_free_cloned(pkt->netBufList);
}

static void NTAPI fort_packet_inject_complete(
        PFORT_PACKET_IO pkt, PNET_BUFFER_LIST clonedNetBufList, BOOLEAN dispatchLevel)
{
    UNUSED(clonedNetBufList);
    UNUSED(dispatchLevel);

    FORT_CHECK_STACK(FORT_PACKET_INJECT_COMPLETE);

    switch (pkt->flags & FORT_PACKET_TYPE_MASK) {
    case FORT_PACKET_TYPE_FLOW: {
        fort_shaper_packet_free(&fort_device()->shaper, (PFORT_FLOW_PACKET) pkt);
    } break;
    case FORT_PACKET_TYPE_PENDING: {
        fort_pending_packet_free((PFORT_PENDING_PACKET) pkt);
    } break;
    }

    InterlockedDecrement(&fort_device()->pending.inject_count);
}

static NTSTATUS fort_packet_inject_in(
        PFORT_PACKET_IO pkt, HANDLE injection_id, ADDRESS_FAMILY addressFamily)
{
    PCFORT_PACKET_IN pkt_in = &pkt->in;

    return FwpsInjectTransportReceiveAsync0(injection_id, NULL, NULL, 0, addressFamily,
            pkt->compartmentId, pkt_in->interfaceIndex, pkt_in->subInterfaceIndex, pkt->netBufList,
            (FWPS_INJECT_COMPLETE0) &fort_packet_inject_complete, pkt);
}

static NTSTATUS fort_packet_inject_out(
        PFORT_PACKET_IO pkt, HANDLE injection_id, ADDRESS_FAMILY addressFamily)
{
    PFORT_PACKET_OUT pkt_out = &pkt->out;

    FWPS_TRANSPORT_SEND_PARAMS0 sendArgs;
    RtlZeroMemory(&sendArgs, sizeof(FWPS_TRANSPORT_SEND_PARAMS0));

    sendArgs.remoteAddress = (UCHAR *) &pkt_out->remoteAddr;
    sendArgs.remoteScopeId = pkt_out->remoteScopeId;
    sendArgs.controlData = pkt_out->controlData;
    sendArgs.controlDataLength = pkt_out->controlDataLength;

    return FwpsInjectTransportSendAsync0(injection_id, NULL, pkt_out->endpointHandle, 0, &sendArgs,
            addressFamily, pkt->compartmentId, pkt->netBufList,
            (FWPS_INJECT_COMPLETE0) &fort_packet_inject_complete, pkt);
}

static NTSTATUS fort_packet_clone(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IO pkt)
{
    NTSTATUS status;

    ULONG bytesRetreated = 0;
    if (ca->inbound) {
        bytesRetreated = ca->inMetaValues->transportHeaderSize + ca->inMetaValues->ipHeaderSize;
    }

    if (bytesRetreated != 0) {
        status = NdisRetreatNetBufferDataStart(
                NET_BUFFER_LIST_FIRST_NB(ca->netBufList), bytesRetreated, 0, 0);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = FwpsAllocateCloneNetBufferList0(ca->netBufList, NULL, NULL, 0, &pkt->netBufList);

    if (bytesRetreated != 0) {
        NdisAdvanceNetBufferDataStart(
                NET_BUFFER_LIST_FIRST_NB(ca->netBufList), bytesRetreated, FALSE, 0);
    }

    return status;
}

FORT_API NTSTATUS fort_packet_inject(PFORT_PACKET_IO pkt)
{
    NTSTATUS status;

    const BOOL inbound = (pkt->flags & FORT_PACKET_INBOUND) != 0;
    const BOOL isIPv6 = (pkt->flags & FORT_PACKET_IP6) != 0;
    const ADDRESS_FAMILY addressFamily = (isIPv6 ? AF_INET6 : AF_INET);
    const HANDLE injection_id = fort_packet_injection_id(isIPv6, inbound);

    /* The injection's completion is called asynchronously on success */
    InterlockedIncrement(&fort_device()->pending.inject_count);

    status = inbound ? fort_packet_inject_in(pkt, injection_id, addressFamily)
                     : fort_packet_inject_out(pkt, injection_id, addressFamily);

    if (!NT_SUCCESS(status)) {
        InterlockedDecrement(&fort_device()->pending.inject_count);

        LOG("Shaper: Packet injection call error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_INJECTION_CALL_ERROR, status, 0, 0);
    }

    return status;
}

inline static void fort_packet_fill_in_interface_indexes(
        PCFORT_CALLOUT_ARG ca, int *interfaceField, int *subInterfaceField)
{
    switch (ca->inFixedValues->layerId) {
    case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
        *interfaceField = FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_ALE_AUTH_CONNECT_V4_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
        *interfaceField = FWPS_FIELD_ALE_AUTH_CONNECT_V6_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_ALE_AUTH_CONNECT_V6_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
        *interfaceField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
        *interfaceField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_INBOUND_TRANSPORT_V4:
        *interfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX;
        break;
    case FWPS_LAYER_INBOUND_TRANSPORT_V6:
        *interfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V6_INTERFACE_INDEX;
        *subInterfaceField = FWPS_FIELD_INBOUND_TRANSPORT_V6_SUB_INTERFACE_INDEX;
        break;
    default:
        assert(0);
    }
}

inline static NTSTATUS fort_packet_fill_in(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IN pkt_in)
{
    int interfaceField;
    int subInterfaceField;
    fort_packet_fill_in_interface_indexes(ca, &interfaceField, &subInterfaceField);

    pkt_in->interfaceIndex = ca->inFixedValues->incomingValue[interfaceField].value.uint32;
    pkt_in->subInterfaceIndex = ca->inFixedValues->incomingValue[subInterfaceField].value.uint32;

    return STATUS_SUCCESS;
}

inline static void fort_packet_fill_out_remoteIp_index(PCFORT_CALLOUT_ARG ca, int *remoteIpField)
{
    switch (ca->inFixedValues->layerId) {
    case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
        *remoteIpField = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
        *remoteIpField = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
        *remoteIpField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
        *remoteIpField = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
        *remoteIpField = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS;
        break;
    case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
        *remoteIpField = FWPS_FIELD_OUTBOUND_TRANSPORT_V6_IP_REMOTE_ADDRESS;
        break;
    default:
        NT_ASSERT(0);
    }
}

inline static NTSTATUS fort_packet_fill_out_controlData(
        PCFORT_CALLOUT_ARG ca, PFORT_PACKET_OUT pkt_out)
{
    if (!FWPS_IS_METADATA_FIELD_PRESENT(
                ca->inMetaValues, FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA))
        return STATUS_SUCCESS;

    const ULONG controlDataLength = ca->inMetaValues->controlDataLength;
    if (controlDataLength == 0)
        return STATUS_SUCCESS;

    pkt_out->controlData = fort_mem_alloc(controlDataLength, FORT_PACKET_POOL_TAG);
    if (pkt_out->controlData == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(pkt_out->controlData, ca->inMetaValues->controlData, controlDataLength);

    pkt_out->controlDataLength = controlDataLength;

    return STATUS_SUCCESS;
}

inline static NTSTATUS fort_packet_fill_out(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_OUT pkt_out)
{
    const NTSTATUS status = fort_packet_fill_out_controlData(ca, pkt_out);
    if (!NT_SUCCESS(status))
        return status;

    pkt_out->remoteScopeId = ca->inMetaValues->remoteScopeId;
    pkt_out->endpointHandle = ca->inMetaValues->transportEndpointHandle;

    int remoteIpField;
    fort_packet_fill_out_remoteIp_index(ca, &remoteIpField);

    const FWP_VALUE0 *remoteIpValue = &ca->inFixedValues->incomingValue[remoteIpField].value;
    if (ca->isIPv6) {
        pkt_out->remoteAddr.v6 = *((ip6_addr_t *) remoteIpValue->byteArray16);
    } else {
        /* host-order -> network-order conversion */
        pkt_out->remoteAddr.v4 = HTONL(remoteIpValue->uint32);
    }

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS fort_packet_fill(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IO pkt, UCHAR pkt_flags)
{
    NTSTATUS status;

    status = ca->inbound ? fort_packet_fill_in(ca, &pkt->in) : fort_packet_fill_out(ca, &pkt->out);

    if (!NT_SUCCESS(status))
        return status;

    pkt->flags = (ca->inbound ? FORT_PACKET_INBOUND : 0) | (ca->isIPv6 ? FORT_PACKET_IP6 : 0)
            | pkt_flags;

    pkt->compartmentId = ca->inMetaValues->compartmentId;

    status = fort_packet_clone(ca, pkt);

    if (!NT_SUCCESS(status)) {
        LOG("Shaper: Packet clone error: %x\n", status);
        TRACE(FORT_SHAPER_PACKET_CLONE_ERROR, status, 0, 0);
    }

    return status;
}
