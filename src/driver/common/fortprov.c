/* Fort Firewall Driver Provider (Un)Registration */

#include "fortdef.h"
#include "fortprov.h"

FORT_API DWORD fort_prov_trans_close(HANDLE transEngine, DWORD status)
{
    if (NT_SUCCESS(status)) {
        status = fort_prov_trans_commit(transEngine);
    } else {
        fort_prov_trans_abort(transEngine);
    }

    fort_prov_close(transEngine);

    return status;
}

static void fort_prov_unregister_callouts(HANDLE engine)
{
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_CONNECT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_ACCEPT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_STREAM_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_DATAGRAM_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_OUT_TRANSPORT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_IN);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_OUT);
    FwpmSubLayerDeleteByKey0(engine, (GUID *) &FORT_GUID_SUBLAYER);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_CONNECT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_ACCEPT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_STREAM_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_DATAGRAM_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_TRANSPORT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_OUT_TRANSPORT_V4);
    FwpmProviderDeleteByKey0(engine, (GUID *) &FORT_GUID_PROVIDER);
}

FORT_API void fort_prov_unregister(HANDLE transEngine)
{
    HANDLE engine = transEngine;

    if (!transEngine) {
        if (fort_prov_open(&engine))
            return;

        fort_prov_trans_begin(engine);
    }

    fort_prov_unregister_callouts(engine);

    if (!transEngine) {
        fort_prov_trans_commit(engine);
        fort_prov_close(engine);
    }
}

static void fort_prov_flow_unregister_callouts(HANDLE engine)
{
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_STREAM_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_DATAGRAM_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_OUT_TRANSPORT_V4);
}

FORT_API void fort_prov_flow_unregister(HANDLE transEngine)
{
    HANDLE engine = transEngine;

    if (!transEngine) {
        if (fort_prov_open(&engine))
            return;

        fort_prov_trans_begin(engine);
    }

    fort_prov_flow_unregister_callouts(engine);

    if (!transEngine) {
        fort_prov_trans_close(engine, 0);
    }
}

FORT_API BOOL fort_prov_is_boot(void)
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

static DWORD fort_prov_register_callouts(HANDLE engine, BOOL is_boot)
{
    const UINT32 filter_flags = is_boot ? 0 : FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED;

    FWPM_PROVIDER0 provider;
    RtlZeroMemory(&provider, sizeof(FWPM_PROVIDER0));
    provider.flags = is_boot ? FWPM_PROVIDER_FLAG_PERSISTENT : 0;
    provider.providerKey = FORT_GUID_PROVIDER;
    provider.displayData.name = (PWCHAR) L"FortProvider";
    provider.displayData.description = (PWCHAR) L"Fort Firewall Provider";
    provider.serviceName = (PWCHAR) L"fortfw";

    FWPM_CALLOUT0 ocallout4;
    RtlZeroMemory(&ocallout4, sizeof(FWPM_CALLOUT0));
    ocallout4.calloutKey = FORT_GUID_CALLOUT_CONNECT_V4;
    ocallout4.displayData.name = (PWCHAR) L"FortCalloutConnect4";
    ocallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Connect V4";
    ocallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
    ocallout4.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V4;

    FWPM_CALLOUT0 icallout4;
    RtlZeroMemory(&icallout4, sizeof(FWPM_CALLOUT0));
    icallout4.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4;
    icallout4.displayData.name = (PWCHAR) L"FortCalloutAccept4";
    icallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Accept V4";
    icallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
    icallout4.applicableLayer = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;

    FWPM_CALLOUT0 scallout4;
    RtlZeroMemory(&scallout4, sizeof(FWPM_CALLOUT0));
    scallout4.calloutKey = FORT_GUID_CALLOUT_STREAM_V4;
    scallout4.displayData.name = (PWCHAR) L"FortCalloutStream4";
    scallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Stream V4";
    scallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
    scallout4.applicableLayer = FWPM_LAYER_STREAM_V4;

    FWPM_CALLOUT0 dcallout4;
    RtlZeroMemory(&dcallout4, sizeof(FWPM_CALLOUT0));
    dcallout4.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V4;
    dcallout4.displayData.name = (PWCHAR) L"FortCalloutDatagram4";
    dcallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Datagram V4";
    dcallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
    dcallout4.applicableLayer = FWPM_LAYER_DATAGRAM_DATA_V4;

    FWPM_CALLOUT0 itcallout4;
    RtlZeroMemory(&itcallout4, sizeof(FWPM_CALLOUT0));
    itcallout4.calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V4;
    itcallout4.displayData.name = (PWCHAR) L"FortCalloutInTransport4";
    itcallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Inbound Transport V4";
    itcallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
    itcallout4.applicableLayer = FWPM_LAYER_INBOUND_TRANSPORT_V4;

    FWPM_CALLOUT0 otcallout4;
    RtlZeroMemory(&otcallout4, sizeof(FWPM_CALLOUT0));
    otcallout4.calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V4;
    otcallout4.displayData.name = (PWCHAR) L"FortCalloutOutTransport4";
    otcallout4.displayData.description = (PWCHAR) L"Fort Firewall Callout Outbound Transport V4";
    otcallout4.providerKey = (GUID *) &FORT_GUID_PROVIDER;
    otcallout4.applicableLayer = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;

    FWPM_SUBLAYER0 sublayer;
    RtlZeroMemory(&sublayer, sizeof(FWPM_SUBLAYER0));
    sublayer.subLayerKey = FORT_GUID_SUBLAYER;
    sublayer.displayData.name = (PWCHAR) L"FortSublayer";
    sublayer.displayData.description = (PWCHAR) L"Fort Firewall Sublayer";
    sublayer.providerKey = (GUID *) &FORT_GUID_PROVIDER;

    FWPM_FILTER0 ofilter4;
    RtlZeroMemory(&ofilter4, sizeof(FWPM_FILTER0));
    ofilter4.flags = filter_flags;
    ofilter4.filterKey = FORT_GUID_FILTER_CONNECT_V4;
    ofilter4.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    ofilter4.subLayerKey = FORT_GUID_SUBLAYER;
    ofilter4.displayData.name = (PWCHAR) L"FortFilterConnect4";
    ofilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Connect V4";
    ofilter4.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
    ofilter4.action.calloutKey = FORT_GUID_CALLOUT_CONNECT_V4;

    FWPM_FILTER0 ifilter4;
    RtlZeroMemory(&ifilter4, sizeof(FWPM_FILTER0));
    ifilter4.flags = filter_flags;
    ifilter4.filterKey = FORT_GUID_FILTER_ACCEPT_V4;
    ifilter4.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
    ifilter4.subLayerKey = FORT_GUID_SUBLAYER;
    ifilter4.displayData.name = (PWCHAR) L"FortFilterAccept4";
    ifilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Accept V4";
    ifilter4.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
    ifilter4.action.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4;

    DWORD status;
    if ((status = FwpmProviderAdd0(engine, &provider, NULL))
            || (status = FwpmCalloutAdd0(engine, &ocallout4, NULL, NULL))
            || (status = FwpmCalloutAdd0(engine, &icallout4, NULL, NULL))
            || (status = FwpmCalloutAdd0(engine, &scallout4, NULL, NULL))
            || (status = FwpmCalloutAdd0(engine, &dcallout4, NULL, NULL))
            || (status = FwpmCalloutAdd0(engine, &itcallout4, NULL, NULL))
            || (status = FwpmCalloutAdd0(engine, &otcallout4, NULL, NULL))
            || (status = FwpmSubLayerAdd0(engine, &sublayer, NULL))
            || (status = FwpmFilterAdd0(engine, &ofilter4, NULL, NULL))
            || (status = FwpmFilterAdd0(engine, &ifilter4, NULL, NULL))) {
        return status;
    }

    return 0;
}

FORT_API DWORD fort_prov_register(HANDLE transEngine, BOOL is_boot)
{
    HANDLE engine = transEngine;
    DWORD status;

    if (!transEngine) {
        if ((status = fort_prov_open(&engine)))
            return status;

        fort_prov_trans_begin(engine);
    }

    status = fort_prov_register_callouts(engine, is_boot);

    if (!transEngine) {
        status = fort_prov_trans_close(transEngine, status);
    }

    return status;
}

static DWORD fort_prov_flow_register_callouts(HANDLE engine, BOOL filter_transport)
{
    const UINT32 filter_flags = FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED
            | FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION;

    FWPM_FILTER0 sfilter4;
    RtlZeroMemory(&sfilter4, sizeof(FWPM_FILTER0));
    sfilter4.flags = filter_flags;
    sfilter4.filterKey = FORT_GUID_FILTER_STREAM_V4;
    sfilter4.layerKey = FWPM_LAYER_STREAM_V4;
    sfilter4.subLayerKey = FORT_GUID_SUBLAYER;
    sfilter4.displayData.name = (PWCHAR) L"FortFilterStream4";
    sfilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Stream V4";
    sfilter4.action.type = FWP_ACTION_CALLOUT_TERMINATING;
    sfilter4.action.calloutKey = FORT_GUID_CALLOUT_STREAM_V4;

    FWPM_FILTER0 dfilter4;
    RtlZeroMemory(&dfilter4, sizeof(FWPM_FILTER0));
    dfilter4.flags = filter_flags;
    dfilter4.filterKey = FORT_GUID_FILTER_DATAGRAM_V4;
    dfilter4.layerKey = FWPM_LAYER_DATAGRAM_DATA_V4;
    dfilter4.subLayerKey = FORT_GUID_SUBLAYER;
    dfilter4.displayData.name = (PWCHAR) L"FortFilterDatagram4";
    dfilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Datagram V4";
    dfilter4.action.type = FWP_ACTION_CALLOUT_TERMINATING;
    dfilter4.action.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V4;

    FWPM_FILTER0 itfilter4;
    RtlZeroMemory(&itfilter4, sizeof(FWPM_FILTER0));
    itfilter4.flags = filter_flags;
    itfilter4.filterKey = FORT_GUID_FILTER_IN_TRANSPORT_V4;
    itfilter4.layerKey = FWPM_LAYER_INBOUND_TRANSPORT_V4;
    itfilter4.subLayerKey = FORT_GUID_SUBLAYER;
    itfilter4.displayData.name = (PWCHAR) L"FortFilterInTransport4";
    itfilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Inbound Transport V4";
    itfilter4.action.type = FWP_ACTION_CALLOUT_TERMINATING;
    itfilter4.action.calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V4;

    FWPM_FILTER0 otfilter4;
    RtlZeroMemory(&otfilter4, sizeof(FWPM_FILTER0));
    otfilter4.flags = filter_flags;
    otfilter4.filterKey = FORT_GUID_FILTER_OUT_TRANSPORT_V4;
    otfilter4.layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
    otfilter4.subLayerKey = FORT_GUID_SUBLAYER;
    otfilter4.displayData.name = (PWCHAR) L"FortFilterOutTransport4";
    otfilter4.displayData.description = (PWCHAR) L"Fort Firewall Filter Outbound Transport V4";
    otfilter4.action.type = FWP_ACTION_CALLOUT_TERMINATING;
    otfilter4.action.calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V4;

    DWORD status;
    if ((status = FwpmFilterAdd0(engine, &sfilter4, NULL, NULL))
            || (status = FwpmFilterAdd0(engine, &dfilter4, NULL, NULL))
            || (filter_transport
                    && ((status = FwpmFilterAdd0(engine, &itfilter4, NULL, NULL))
                            || (status = FwpmFilterAdd0(engine, &otfilter4, NULL, NULL))))) {
        return status;
    }

    return 0;
}

FORT_API DWORD fort_prov_flow_register(HANDLE transEngine, BOOL filter_transport)
{
    HANDLE engine = transEngine;
    DWORD status;

    if (!transEngine) {
        if ((status = fort_prov_open(&engine)))
            return status;

        fort_prov_trans_begin(engine);
    }

    status = fort_prov_flow_register_callouts(engine, filter_transport);

    if (!transEngine) {
        status = fort_prov_trans_close(transEngine, status);
    }

    return status;
}

static DWORD fort_prov_reauth_callouts(HANDLE engine)
{
    FWPM_FILTER0 ifilter;
    RtlZeroMemory(&ifilter, sizeof(FWPM_FILTER0));
    ifilter.filterKey = FORT_GUID_FILTER_REAUTH_IN;
    ifilter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
    ifilter.subLayerKey = FORT_GUID_SUBLAYER;
    ifilter.displayData.name = (PWCHAR) L"FortFilterReauthIn";
    ifilter.displayData.description = (PWCHAR) L"Fort Firewall Filter Reauth Inbound";
    ifilter.action.type = FWP_ACTION_CONTINUE;

    FWPM_FILTER0 ofilter;
    RtlZeroMemory(&ofilter, sizeof(FWPM_FILTER0));
    ofilter.filterKey = FORT_GUID_FILTER_REAUTH_OUT;
    ofilter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    ofilter.subLayerKey = FORT_GUID_SUBLAYER;
    ofilter.displayData.name = (PWCHAR) L"FortFilterReauthOut";
    ofilter.displayData.description = (PWCHAR) L"Fort Firewall Filter Reauth Outbound";
    ofilter.action.type = FWP_ACTION_CONTINUE;

    DWORD status;
    if ((status = FwpmFilterAdd0(engine, &ifilter, NULL, NULL))
            || (status = FwpmFilterAdd0(engine, &ofilter, NULL, NULL))) {
        return status;
    }

    return 0;
}

FORT_API DWORD fort_prov_reauth(HANDLE transEngine)
{
    HANDLE engine = transEngine;
    DWORD status;

    if (!transEngine) {
        if ((status = fort_prov_open(&engine)))
            return status;

        fort_prov_trans_begin(engine);
    }

    status = FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_IN);
    if (NT_SUCCESS(status)) {
        FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_OUT);
    } else {
        status = fort_prov_reauth_callouts(engine);
    }

    if (!transEngine) {
        status = fort_prov_trans_close(transEngine, status);
    }

    return status;
}
