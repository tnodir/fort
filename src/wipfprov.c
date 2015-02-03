/* Windows IP Filter Provider (Un)Registration */

typedef struct wipf_prov_data {
  UINT32 version	: 24;
  UINT32 persist	: 1;
  UINT32 boot		: 1;
} WIPF_PROV_DATA, *PWIPF_PROV_DATA;


static void
wipf_prov_delete (HANDLE engine)
{
  FwpmFilterDeleteByKey0(engine, (GUID *) &WIPF_GUID_FILTER_CONNECT_V4);
  FwpmFilterDeleteByKey0(engine, (GUID *) &WIPF_GUID_FILTER_ACCEPT_V4);
  FwpmSubLayerDeleteByKey0(engine, (GUID *) &WIPF_GUID_SUBLAYER);
  FwpmCalloutDeleteByKey0(engine, (GUID *) &WIPF_GUID_CALLOUT_CONNECT_V4);
  FwpmCalloutDeleteByKey0(engine, (GUID *) &WIPF_GUID_CALLOUT_ACCEPT_V4);
  FwpmProviderDeleteByKey0(engine, (GUID *) &WIPF_GUID_PROVIDER);
}

static void
wipf_prov_unregister (void)
{
  HANDLE engine;

  if (FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &engine))
    return;

  wipf_prov_delete(engine);

  FwpmEngineClose0(engine);
}

static DWORD
wipf_prov_register (BOOL persist, BOOL boot, BOOL *is_tempp)
{
  FWPM_PROVIDER0 *old_provider, provider;
  FWPM_CALLOUT0 ocallout4, icallout4;
  FWPM_SUBLAYER0 sublayer;
  FWPM_FILTER0 ofilter4, ifilter4;
  HANDLE engine;
  WIPF_PROV_DATA provider_data;
  UINT32 filter_flags;
  DWORD status;

  if ((status = FwpmEngineOpen0(
      NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &engine)))
    goto end;

  if (!(status = FwpmProviderGetByKey0(
      engine, (GUID *) &WIPF_GUID_PROVIDER, &old_provider))) {
    PWIPF_PROV_DATA old_provider_data =
        (PWIPF_PROV_DATA) old_provider->providerData.data;

    if (old_provider_data) {
      provider_data = *old_provider_data;
    }
    FwpmFreeMemory0(&old_provider);

    if (old_provider_data) {
      if (provider_data.persist) {
        if (provider_data.version == WIPF_VERSION)
          goto end_close;

        persist = provider_data.persist;
        boot = provider_data.boot;
      }
      wipf_prov_delete(engine);
    }
  }

  provider_data.version = WIPF_VERSION;
  provider_data.persist = persist;
  provider_data.boot = boot;

  filter_flags = boot ? 0 : FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED;

  RtlZeroMemory(&provider, sizeof(FWPM_PROVIDER0));
  provider.flags = persist ? FWPM_PROVIDER_FLAG_PERSISTENT : 0;
  provider.providerKey = WIPF_GUID_PROVIDER;
  provider.displayData.name = L"WipfProvider";
  provider.displayData.description = L"Windows IP Filter Provider";
  provider.serviceName = L"wipf";
  provider.providerData.size = sizeof(WIPF_PROV_DATA);
  provider.providerData.data = (UINT8 *) &provider_data;

  RtlZeroMemory(&ocallout4, sizeof(FWPM_CALLOUT0));
  ocallout4.calloutKey = WIPF_GUID_CALLOUT_CONNECT_V4;
  ocallout4.displayData.name = L"WipfCalloutConnect4";
  ocallout4.displayData.description = L"Windows IP Filter Callout Connect V4";
  ocallout4.providerKey = (GUID *) &WIPF_GUID_PROVIDER;
  ocallout4.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V4;

  RtlZeroMemory(&icallout4, sizeof(FWPM_CALLOUT0));
  icallout4.calloutKey = WIPF_GUID_CALLOUT_ACCEPT_V4;
  icallout4.displayData.name = L"WipfCalloutAccept4";
  icallout4.displayData.description = L"Windows IP Filter Callout Accept V4";
  icallout4.providerKey = (GUID *) &WIPF_GUID_PROVIDER;
  icallout4.applicableLayer = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;

  RtlZeroMemory(&sublayer, sizeof(FWPM_SUBLAYER0));
  sublayer.subLayerKey = WIPF_GUID_SUBLAYER;
  sublayer.displayData.name = L"WipfSublayer";
  sublayer.displayData.description = L"Windows IP Filter Sublayer";
  sublayer.providerKey = (GUID *) &WIPF_GUID_PROVIDER;

  RtlZeroMemory(&ofilter4, sizeof(FWPM_FILTER0));
  ofilter4.flags = filter_flags;
  ofilter4.filterKey = WIPF_GUID_FILTER_CONNECT_V4;
  ofilter4.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
  ofilter4.subLayerKey = WIPF_GUID_SUBLAYER;
  ofilter4.displayData.name = L"WipfFilterConnect4";
  ofilter4.displayData.description = L"Windows IP Filter Connect V4";
  ofilter4.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
  ofilter4.action.calloutKey = WIPF_GUID_CALLOUT_CONNECT_V4;

  RtlZeroMemory(&ifilter4, sizeof(FWPM_FILTER0));
  ifilter4.flags = filter_flags;
  ifilter4.filterKey = WIPF_GUID_FILTER_ACCEPT_V4;
  ifilter4.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
  ifilter4.subLayerKey = WIPF_GUID_SUBLAYER;
  ifilter4.displayData.name = L"WipfFilterAccept4";
  ifilter4.displayData.description = L"Windows IP Filter Accept V4";
  ifilter4.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
  ifilter4.action.calloutKey = WIPF_GUID_CALLOUT_ACCEPT_V4;

  if ((status = FwpmTransactionBegin0(engine, 0))
      || (status = FwpmProviderAdd0(engine, &provider, NULL))
      || (status = FwpmCalloutAdd0(engine, &ocallout4, NULL, NULL))
      || (status = FwpmCalloutAdd0(engine, &icallout4, NULL, NULL))
      || (status = FwpmSubLayerAdd0(engine, &sublayer, NULL))
      || (status = FwpmFilterAdd0(engine, &ofilter4, NULL, NULL))
      || (status = FwpmFilterAdd0(engine, &ifilter4, NULL, NULL))
      || (status = FwpmTransactionCommit0(engine))) {
    FwpmTransactionAbort0(engine);
  } else if (is_tempp) {
    *is_tempp = !persist;
  }

 end_close:
  FwpmEngineClose0(engine);

 end:
  return status;
}

