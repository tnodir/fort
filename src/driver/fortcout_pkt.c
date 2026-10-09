/* Fort Firewall Callouts: Packet */

#include "fortcout_pkt.h"

#include "fortcout.h"
#include "fortcoutarg.h"
#include "fortdbg.h"
#include "fortdev.h"

inline static UINT32 fort_packet_data_size(const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
        const PNET_BUFFER_LIST netBufList, BOOL inbound)
{
    if (netBufList == NULL)
        return 0;

    const UINT32 headerSize =
            inbound ? inMetaValues->ipHeaderSize + inMetaValues->transportHeaderSize : 0;

    UINT32 dataSize = 0;

    /* The packets list may contain several segments */
    PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList);
    for (; netBuf != NULL; netBuf = NET_BUFFER_NEXT_NB(netBuf)) {
        dataSize += NET_BUFFER_DATA_LENGTH(netBuf) + headerSize;
    }

    return dataSize;
}

inline static BOOL fort_packet_layer_is_ipv6(UINT16 layerId)
{
    return layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6
            || layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6;
}

inline static BOOL fort_callout_transport_classify_shaper(
        FWPS_CLASSIFY_OUT0 *classifyOut, PFORT_CALLOUT_ARG ca)
{
    if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
        return FALSE; /* Can't act on the packet */

    if (fort_shaper_packet_process(&fort_device()->shaper, ca)) {
        fort_callout_classify_drop(classifyOut); /* drop */
        return TRUE;
    }

    return FALSE;
}

inline static BOOL fort_callout_transport_classify_blocked(const FWPS_CLASSIFY_OUT0 *classifyOut)
{
    return (classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0
            && classifyOut->actionType == FWP_ACTION_BLOCK;
}

inline static BOOL fort_callout_transport_classify_skip_reinjected(PCFORT_CALLOUT_ARG ca)
{
    /* Skip the packet re-injected by another driver: its original packet is counted */
    const FORT_CONF_FLAGS conf_flags = fort_device_conf_flags(&fort_device()->conf);
    if (conf_flags.log_stat_reinjected)
        return FALSE;

    return fort_packet_injected_by_other(ca);
}

static void fort_callout_transport_classify_stat(
        const FWPS_CLASSIFY_OUT0 *classifyOut, PCFORT_CALLOUT_ARG ca)
{
    /* Skip the packet blocked or absorbed (to be re-injected) by a higher sublayer's callout */
    if (fort_callout_transport_classify_blocked(classifyOut))
        return;

    if (fort_callout_transport_classify_skip_reinjected(ca))
        return;

    PFORT_STAT stat = &fort_device()->stat;

    if (!fort_flow_classify(stat, ca->flowContext, ca->dataSize, ca->inbound)) {
        /* Flush the traffic statistics on the process's bytes' overflow */
        fort_callout_timer();

        fort_flow_classify(stat, ca->flowContext, ca->dataSize, ca->inbound);
    }
}

static void fort_callout_transport_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        BOOL inbound)
{
    FORT_CHECK_STACK(FORT_CALLOUT_TRANSPORT_CLASSIFY);

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
        .isIPv6 = fort_packet_layer_is_ipv6(inFixedValues->layerId),
    };

    if (fort_callout_transport_classify_shaper(classifyOut, &ca))
        return;

    fort_callout_transport_classify_stat(classifyOut, &ca);

    fort_callout_classify_continue(classifyOut); /* continue */
}

FORT_API void NTAPI fort_callout_transport_classify_in(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_transport_classify(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, /*inbound=*/TRUE);
}

FORT_API void NTAPI fort_callout_transport_classify_out(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_transport_classify(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, /*inbound=*/FALSE);
}

FORT_API void NTAPI fort_callout_flow_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);

    FORT_CHECK_STACK(FORT_CALLOUT_FLOW_DELETE);

    fort_shaper_drop_flow_packets(&fort_device()->shaper, flowContext);

    fort_flow_delete(&fort_device()->stat, flowContext);
}

static void fort_callout_discard_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut,
        UCHAR flagsIndex)
{
    UNUSED(inMetaValues);
    UNUSED(layerData);
    UNUSED(flowContext);

    FORT_CHECK_STACK(FORT_CALLOUT_DISCARD_CLASSIFY);

    const UINT32 classify_flags = inFixedValues->incomingValue[flagsIndex].value.uint32;
    const BOOL is_loopback = (classify_flags & FWP_CONDITION_FLAG_IS_LOOPBACK) != 0;

    if (is_loopback) {
        fort_callout_classify_permit(filter, classifyOut); /* permit */
    } else {
        fort_callout_classify_block(classifyOut); /* block */
    }
}

FORT_API void NTAPI fort_callout_transport_discard_in_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_discard_classify(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, FWPS_FIELD_INBOUND_TRANSPORT_V4_FLAGS);
}

FORT_API void NTAPI fort_callout_transport_discard_in_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_discard_classify(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, FWPS_FIELD_INBOUND_TRANSPORT_V6_FLAGS);
}

FORT_API void NTAPI fort_callout_ippacket_discard_in_v4(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_discard_classify(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, FWPS_FIELD_INBOUND_IPPACKET_V4_FLAGS);
}

FORT_API void NTAPI fort_callout_ippacket_discard_in_v6(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    fort_callout_discard_classify(inFixedValues, inMetaValues, layerData, filter, flowContext,
            classifyOut, FWPS_FIELD_INBOUND_IPPACKET_V6_FLAGS);
}
