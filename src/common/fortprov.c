/* Fort Firewall Driver Provider (Un)Registration */

static DWORD
fort_prov_open (HANDLE *enginep)
{
  return FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, enginep);
}

static void
fort_prov_close (HANDLE engine)
{
  FwpmEngineClose0(engine);
}

static void
fort_prov_unregister (void)
{
  HANDLE engine;

  if (fort_prov_open(&engine))
    return;

  FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_CONNECT_V4);
  FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_ACCEPT_V4);
  FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_FLOW_V4);
  FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH);
  FwpmSubLayerDeleteByKey0(engine, (GUID *) &FORT_GUID_SUBLAYER);
  FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_CONNECT_V4);
  FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_ACCEPT_V4);
  FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_FLOW_V4);
  FwpmProviderDeleteByKey0(engine, (GUID *) &FORT_GUID_PROVIDER);

  fort_prov_close(engine);
}

static void
fort_prov_flow_unregister (void)
{
  HANDLE engine;

  if (fort_prov_open(&engine))
    return;

  FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_FLOW_V4);

  fort_prov_close(engine);
}

static DWORD
fort_prov_register (BOOL is_boot)
{
  FWPM_PROVIDER0 provider;
  FWPM_CALLOUT0 ocallout4, icallout4, fcallout4;
  FWPM_SUBLAYER0 sublayer;
  FWPM_FILTER0 ofilter4, ifilter4;
  HANDLE engine;
  UINT32 filter_flags;
  DWORD status;

  if ((status = fort_prov_open(&engine)))
    goto end;

  filter_flags = is_boot ? 0 : FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED;

  RtlZeroMemory(&provider, sizeof(FWPM_PROVIDER0));
  provider.flags = is_boot ? FWPM_PROVIDER_FLAG_PERSISTENT : 0;
  provider.providerKey = FORT_GUID_PROVIDER;
  provider.displayData.name = (PWCHAR) L"FortProvider";
  provider.displayData.description = (PWCHAR) L"Fort Firewall Provider";
  provider.serviceName = (PWCHAR) L"fortfw";

  RtlZeroMemory(&ocallout4, sizeof(FWPM_CALLOUT0));
  ocallout4.calloutKey = FORT_GUID_CALLOUT_CONNECT_V4;
  ocallout4.displayData.name = (PWCHAR) L"FortCalloutConnect4";
  ocallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Connect V4";
  ocallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
  ocallout4.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V4;

  RtlZeroMemory(&icallout4, sizeof(FWPM_CALLOUT0));
  icallout4.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4;
  icallout4.displayData.name = (PWCHAR) L"FortCalloutAccept4";
  icallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Accept V4";
  icallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
  icallout4.applicableLayer = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;

  RtlZeroMemory(&fcallout4, sizeof(FWPM_CALLOUT0));
  fcallout4.calloutKey = FORT_GUID_CALLOUT_FLOW_V4;
  fcallout4.displayData.name = (PWCHAR) L"FortCalloutFlow4";
  fcallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Flow V4";
  fcallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
  fcallout4.applicableLayer = FWPM_LAYER_STREAM_V4;

  RtlZeroMemory(&sublayer, sizeof(FWPM_SUBLAYER0));
  sublayer.subLayerKey = FORT_GUID_SUBLAYER;
  sublayer.displayData.name = (PWCHAR) L"FortSublayer";
  sublayer.displayData.description = (PWCHAR) L"Fort Firewall Sublayer";
  sublayer.providerKey = (GUID *) &FORT_GUID_PROVIDER;

  RtlZeroMemory(&ofilter4, sizeof(FWPM_FILTER0));
  ofilter4.flags = filter_flags;
  ofilter4.filterKey = FORT_GUID_FILTER_CONNECT_V4;
  ofilter4.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
  ofilter4.subLayerKey = FORT_GUID_SUBLAYER;
  ofilter4.displayData.name = (PWCHAR) L"FortFilterConnect4";
  ofilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Connect V4";
  ofilter4.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
  ofilter4.action.calloutKey = FORT_GUID_CALLOUT_CONNECT_V4;

  RtlZeroMemory(&ifilter4, sizeof(FWPM_FILTER0));
  ifilter4.flags = filter_flags;
  ifilter4.filterKey = FORT_GUID_FILTER_ACCEPT_V4;
  ifilter4.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
  ifilter4.subLayerKey = FORT_GUID_SUBLAYER;
  ifilter4.displayData.name = (PWCHAR) L"FortFilterAccept4";
  ifilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Accept V4";
  ifilter4.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
  ifilter4.action.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4;

  if ((status = FwpmTransactionBegin0(engine, 0))
      || (status = FwpmProviderAdd0(engine, &provider, NULL))
      || (status = FwpmCalloutAdd0(engine, &ocallout4, NULL, NULL))
      || (status = FwpmCalloutAdd0(engine, &icallout4, NULL, NULL))
      || (status = FwpmCalloutAdd0(engine, &fcallout4, NULL, NULL))
      || (status = FwpmSubLayerAdd0(engine, &sublayer, NULL))
      || (status = FwpmFilterAdd0(engine, &ofilter4, NULL, NULL))
      || (status = FwpmFilterAdd0(engine, &ifilter4, NULL, NULL))
      || (status = FwpmTransactionCommit0(engine))) {
    FwpmTransactionAbort0(engine);
  }

  fort_prov_close(engine);

 end:
  return status;
}

static DWORD
fort_prov_flow_register (void)
{
  FWPM_FILTER0 ffilter4;
  HANDLE engine;
  DWORD status;

  if ((status = fort_prov_open(&engine)))
    goto end;

  RtlZeroMemory(&ffilter4, sizeof(FWPM_FILTER0));
  ffilter4.filterKey = FORT_GUID_FILTER_FLOW_V4;
  ffilter4.layerKey = FWPM_LAYER_STREAM_V4;
  ffilter4.subLayerKey = FORT_GUID_SUBLAYER;
  ffilter4.displayData.name = (PWCHAR) L"FortFilterFlow4";
  ffilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Flow V4";
  ffilter4.action.type = FWP_ACTION_CALLOUT_INSPECTION;
  ffilter4.action.calloutKey = FORT_GUID_CALLOUT_FLOW_V4;

  if ((status = FwpmTransactionBegin0(engine, 0))
      || (status = FwpmFilterAdd0(engine, &ffilter4, NULL, NULL))
      || (status = FwpmTransactionCommit0(engine))) {
    FwpmTransactionAbort0(engine);
  }

  fort_prov_close(engine);

 end:
  return status;
}

static BOOL
fort_prov_is_boot (void)
{
  HANDLE engine;
  BOOL is_boot = FALSE;

  if (!fort_prov_open(&engine)) {
    FWPM_PROVIDER0 *provider;

    if (!FwpmProviderGetByKey0(engine, (GUID *) &FORT_GUID_PROVIDER, &provider)) {
      is_boot = (provider->flags & FWPM_PROVIDER_FLAG_PERSISTENT);

      FwpmFreeMemory0((void **) &provider);
    }

    fort_prov_close(engine);
  }

  return is_boot;
}

static DWORD
fort_prov_reauth (void)
{
  FWPM_FILTER0 filter;
  HANDLE engine;
  DWORD status;

  if ((status = fort_prov_open(&engine)))
    return status;

  RtlZeroMemory(&filter, sizeof(FWPM_FILTER0));
  filter.filterKey = FORT_GUID_FILTER_REAUTH;
  filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
  filter.subLayerKey = FORT_GUID_SUBLAYER;
  filter.displayData.name = (PWCHAR) L"FortFilterReauth";
  filter.displayData.description = (PWCHAR) L"Fort Firewall Filter Reauth";
  filter.action.type = FWP_ACTION_CONTINUE;

  if (!FwpmTransactionBegin0(engine, 0)) {
    status = FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH);
    if (status) {
      status = FwpmFilterAdd0(engine, &filter, NULL, NULL);
    }
    FwpmTransactionCommit0(engine);
  }
  
  fort_prov_close(engine);

  return status;
}

