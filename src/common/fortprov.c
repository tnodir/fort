/* Fort Firewall Driver Provider (Un)Registration */

typedef struct fort_prov_data {
  UINT32 version	: 24;
  UINT32 is_boot	: 1;
} FORT_PROV_DATA, *PFORT_PROV_DATA;


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
fort_prov_delete (HANDLE engine)
{
  FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_CONNECT_V4);
  FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_ACCEPT_V4);
  FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_FLOW_V4);
  FwpmSubLayerDeleteByKey0(engine, (GUID *) &FORT_GUID_SUBLAYER);
  FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_CONNECT_V4);
  FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_ACCEPT_V4);
  FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_FLOW_V4);
  FwpmProviderDeleteByKey0(engine, (GUID *) &FORT_GUID_PROVIDER);
}

static void
fort_prov_unregister (void)
{
  HANDLE engine;

  if (fort_prov_open(&engine))
    return;

  fort_prov_delete(engine);

  fort_prov_close(engine);
}

static DWORD
fort_prov_register (BOOL is_boot, BOOL *is_bootp)
{
  FWPM_PROVIDER0 *old_provider, provider;
  FWPM_CALLOUT0 ocallout4, icallout4, fcallout4;
  FWPM_SUBLAYER0 sublayer;
  FWPM_FILTER0 ofilter4, ifilter4, ffilter4;
  HANDLE engine;
  FORT_PROV_DATA provider_data;
  UINT32 filter_flags;
  DWORD status;

  if ((status = fort_prov_open(&engine)))
    goto end;

  if (!(status = FwpmProviderGetByKey0(
      engine, (GUID *) &FORT_GUID_PROVIDER, &old_provider))) {
    PFORT_PROV_DATA old_provider_data =
        (PFORT_PROV_DATA) old_provider->providerData.data;

    if (old_provider_data) {
      provider_data = *old_provider_data;
    }

    FwpmFreeMemory0((void **) &old_provider);

    if (old_provider_data) {
      is_boot = is_bootp ? provider_data.is_boot : is_boot;

      if (is_boot == provider_data.is_boot
          && provider_data.version == APP_VERSION)
        goto end_close;

      fort_prov_delete(engine);
    }
  }

  provider_data.version = APP_VERSION;
  provider_data.is_boot = is_boot;

  filter_flags = is_boot ? 0 : FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED;

  RtlZeroMemory(&provider, sizeof(FWPM_PROVIDER0));
  provider.flags = is_boot ? FWPM_PROVIDER_FLAG_PERSISTENT : 0;
  provider.providerKey = FORT_GUID_PROVIDER;
  provider.displayData.name = (PWCHAR) L"FortProvider";
  provider.displayData.description = (PWCHAR) L"Fort Firewall Provider";
  provider.serviceName = (PWCHAR) L"fortfw";
  provider.providerData.size = sizeof(FORT_PROV_DATA);
  provider.providerData.data = (UINT8 *) &provider_data;

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

  RtlZeroMemory(&ffilter4, sizeof(FWPM_FILTER0));
  ffilter4.filterKey = FORT_GUID_FILTER_FLOW_V4;
  ffilter4.layerKey = FWPM_LAYER_STREAM_V4;
  ffilter4.subLayerKey = FORT_GUID_SUBLAYER;
  ffilter4.displayData.name = (PWCHAR) L"FortFilterFlow4";
  ffilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Flow V4";
  ffilter4.action.type = FWP_ACTION_CALLOUT_INSPECTION;
  ffilter4.action.calloutKey = FORT_GUID_CALLOUT_FLOW_V4;

  if ((status = FwpmTransactionBegin0(engine, 0))
      || (status = FwpmProviderAdd0(engine, &provider, NULL))
      || (status = FwpmCalloutAdd0(engine, &ocallout4, NULL, NULL))
      || (status = FwpmCalloutAdd0(engine, &icallout4, NULL, NULL))
      || (status = FwpmCalloutAdd0(engine, &fcallout4, NULL, NULL))
      || (status = FwpmSubLayerAdd0(engine, &sublayer, NULL))
      || (status = FwpmFilterAdd0(engine, &ofilter4, NULL, NULL))
      || (status = FwpmFilterAdd0(engine, &ifilter4, NULL, NULL))
      || (status = FwpmFilterAdd0(engine, &ffilter4, NULL, NULL))
      || (status = FwpmTransactionCommit0(engine))) {
    FwpmTransactionAbort0(engine);
  }

 end_close:
  fort_prov_close(engine);

  if (is_bootp) {
    *is_bootp = is_boot;
  }

 end:
  return status;
}

