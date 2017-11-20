/* Fort Firewall Usage Statistics */

static void
fort_stat_flow_associate (UINT64 flowId,
                          UINT32 calloutId,
                          UINT32 processId)
{
  NTSTATUS status;

  status = FwpsFlowAssociateContext0(
    flowId, FWPS_LAYER_STREAM_V4, calloutId, processId);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow +: %d %d\n", flowId, processId);

  //STATUS_OBJECT_NAME_EXISTS
}

static void
fort_callout_flow_delete_v4 (UINT16 layerId,
                             UINT32 calloutId,
                             UINT64 flowContext)
{
  UNUSED(layerId);
  UNUSED(calloutId);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow -: %d\n", (UINT32) flowContext);
}

static void
fort_callout_flow_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                               const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                               FWPS_STREAM_CALLOUT_IO_PACKET0 *packet,
                               const FWPS_FILTER0 *filter,
                               UINT64 flowContext,
                               FWPS_CLASSIFY_OUT0 *classifyOut)
{
  FWPS_STREAM_DATA0 *streamData = packet->streamData;

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
             "FORT: flow >: %d %d %d\n", inMetaValues->flowHandle, flowContext, streamData->dataLength);

  classifyOut->actionType = FWP_ACTION_CONTINUE;
}
