#include "um_fwpsk.h"

NTSTATUS NTAPI FwpsCalloutRegister0(
        void *deviceObject, const FWPS_CALLOUT0 *callout, UINT32 *calloutId)
{
    UNUSED(deviceObject);
    UNUSED(callout);
    UNUSED(calloutId);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsCalloutUnregisterById0(const UINT32 calloutId)
{
    UNUSED(calloutId);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsGetPacketListSecurityInformation0(NET_BUFFER_LIST *packetList, UINT32 queryFlags,
        FWPS_PACKET_LIST_INFORMATION0 *packetInformation)
{
    UNUSED(packetList);
    UNUSED(queryFlags);
    UNUSED(packetInformation);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsInjectionHandleCreate0(
        ADDRESS_FAMILY addressFamily, UINT32 flags, HANDLE *injectionHandle)
{
    UNUSED(addressFamily);
    UNUSED(flags);
    UNUSED(injectionHandle);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsInjectionHandleDestroy0(HANDLE injectionHandle)
{
    UNUSED(injectionHandle);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsAllocateCloneNetBufferList0(NET_BUFFER_LIST *originalNetBufferList,
        NDIS_HANDLE netBufferListPoolHandle, NDIS_HANDLE netBufferPoolHandle,
        ULONG allocateCloneFlags, NET_BUFFER_LIST **netBufferList)
{
    UNUSED(originalNetBufferList);
    UNUSED(netBufferListPoolHandle);
    UNUSED(netBufferPoolHandle);
    UNUSED(allocateCloneFlags);
    UNUSED(netBufferList);
    return STATUS_SUCCESS;
}

void NTAPI FwpsFreeCloneNetBufferList0(NET_BUFFER_LIST *netBufferList, ULONG freeCloneFlags)
{
    UNUSED(netBufferList);
    UNUSED(freeCloneFlags);
}

NTSTATUS NTAPI FwpsInjectTransportSendAsync0(HANDLE injectionHandle, HANDLE injectionContext,
        UINT64 endpointHandle, UINT32 flags, FWPS_TRANSPORT_SEND_PARAMS0 *sendArgs,
        ADDRESS_FAMILY addressFamily, COMPARTMENT_ID compartmentId, NET_BUFFER_LIST *netBufferList,
        FWPS_INJECT_COMPLETE0 completionFn, HANDLE completionContext)
{
    UNUSED(injectionHandle);
    UNUSED(injectionContext);
    UNUSED(endpointHandle);
    UNUSED(flags);
    UNUSED(sendArgs);
    UNUSED(addressFamily);
    UNUSED(compartmentId);
    UNUSED(netBufferList);
    UNUSED(completionFn);
    UNUSED(completionContext);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsInjectTransportReceiveAsync0(HANDLE injectionHandle, HANDLE injectionContext,
        PVOID reserved, UINT32 flags, ADDRESS_FAMILY addressFamily, COMPARTMENT_ID compartmentId,
        IF_INDEX interfaceIndex, IF_INDEX subInterfaceIndex, NET_BUFFER_LIST *netBufferList,
        FWPS_INJECT_COMPLETE0 completionFn, HANDLE completionContext)
{
    UNUSED(injectionHandle);
    UNUSED(injectionContext);
    UNUSED(reserved);
    UNUSED(flags);
    UNUSED(addressFamily);
    UNUSED(compartmentId);
    UNUSED(interfaceIndex);
    UNUSED(subInterfaceIndex);
    UNUSED(netBufferList);
    UNUSED(completionFn);
    UNUSED(completionContext);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsInjectMacReceiveAsync0(HANDLE injectionHandle, HANDLE injectionContext,
        UINT32 flags, UINT16 layerId, IF_INDEX interfaceIndex, NDIS_PORT_NUMBER NdisPortNumber,
        NET_BUFFER_LIST *netBufferLists, FWPS_INJECT_COMPLETE completionFn,
        HANDLE completionContext)
{
    UNUSED(injectionHandle);
    UNUSED(injectionContext);
    UNUSED(flags);
    UNUSED(layerId);
    UNUSED(interfaceIndex);
    UNUSED(NdisPortNumber);
    UNUSED(netBufferLists);
    UNUSED(completionFn);
    UNUSED(completionContext);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsInjectMacSendAsync0(HANDLE injectionHandle, HANDLE injectionContext,
        UINT32 flags, UINT16 layerId, IF_INDEX interfaceIndex, NDIS_PORT_NUMBER NdisPortNumber,
        NET_BUFFER_LIST *netBufferLists, FWPS_INJECT_COMPLETE completionFn,
        HANDLE completionContext)
{
    UNUSED(injectionHandle);
    UNUSED(injectionContext);
    UNUSED(flags);
    UNUSED(layerId);
    UNUSED(interfaceIndex);
    UNUSED(NdisPortNumber);
    UNUSED(netBufferLists);
    UNUSED(completionFn);
    UNUSED(completionContext);
    return STATUS_SUCCESS;
}

void NTAPI FwpsReferenceNetBufferList0(NET_BUFFER_LIST *netBufferList, BOOLEAN intendToModify)
{
    UNUSED(netBufferList);
    UNUSED(intendToModify);
}

void NTAPI FwpsDereferenceNetBufferList0(NET_BUFFER_LIST *netBufferList, BOOLEAN dispatchLevel)
{
    UNUSED(netBufferList);
    UNUSED(dispatchLevel);
}

FWPS_PACKET_INJECTION_STATE NTAPI FwpsQueryPacketInjectionState0(
        HANDLE injectionHandle, const NET_BUFFER_LIST *netBufferList, HANDLE *injectionContext)
{
    UNUSED(injectionHandle);
    UNUSED(netBufferList);
    UNUSED(injectionContext);
    return FWPS_PACKET_NOT_INJECTED;
}

NTSTATUS NTAPI FwpsStreamInjectAsync0(HANDLE injectionHandle, HANDLE injectionContext, UINT32 flags,
        UINT64 flowId, UINT32 calloutId, UINT16 layerId, UINT32 streamFlags,
        NET_BUFFER_LIST *netBufferList, SIZE_T dataLength, FWPS_INJECT_COMPLETE0 completionFn,
        HANDLE completionContext)
{
    UNUSED(injectionHandle);
    UNUSED(injectionContext);
    UNUSED(flags);
    UNUSED(flowId);
    UNUSED(calloutId);
    UNUSED(layerId);
    UNUSED(streamFlags);
    UNUSED(netBufferList);
    UNUSED(dataLength);
    UNUSED(completionFn);
    UNUSED(completionContext);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsFlowAssociateContext0(
        UINT64 flowId, UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(flowId);
    UNUSED(layerId);
    UNUSED(calloutId);
    UNUSED(flowContext);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsFlowRemoveContext0(UINT64 flowId, UINT16 layerId, UINT32 calloutId)
{
    UNUSED(flowId);
    UNUSED(layerId);
    UNUSED(calloutId);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI FwpsFlowAbort0(UINT64 flowId)
{
    UNUSED(flowId);
    return STATUS_SUCCESS;
}

NTSTATUS FwpsPendOperation0(HANDLE completionHandle, HANDLE *completionContext)
{
    UNUSED(completionHandle);
    UNUSED(completionContext);
    return STATUS_SUCCESS;
}

void FwpsCompleteOperation0(HANDLE completionContext, PNET_BUFFER_LIST netBufferList)
{
    UNUSED(completionContext);
    UNUSED(netBufferList);
}
