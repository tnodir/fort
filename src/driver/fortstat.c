/* Fort Firewall Usage Statistics */

static void
fort_stat_flow_associate(UINT64 flowId)
{
  //DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
  //           "FORT: Classify V4: %d %d %d\n", FWPS_IS_METADATA_FIELD_PRESENT(
  //             inMetaValues, FWPS_METADATA_FIELD_FLOW_HANDLE),
  //           inMetaValues->flowHandle, (UINT32) flowContext);

  //const NTSTATUS status = FwpsFlowAssociateContext0(
  //  flowId, FWPM_LAYER_STREAM_V4, FORT_GUID_CALLOUT_FLOW_V4, 1);
}

static void
fort_callout_flow_delete_v4 (UINT16 layerId,
                             UINT32 calloutId,
                             UINT64 flowContext)
{
}

static void
fort_callout_flow_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                      const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                      void *layerData,
                      const FWPS_FILTER0 *filter,
                      UINT64 flowContext,
                      FWPS_CLASSIFY_OUT0 *classifyOut)
{
}
