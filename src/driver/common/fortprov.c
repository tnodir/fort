/* Fort Firewall Driver Provider (Un)Registration */

#include "fortprov.h"

#include "fortioctl.h"

#define FORT_PROV_CALLOUTS_COUNT        12
#define FORT_PROV_BOOT_FILTERS_COUNT    4
#define FORT_PROV_PERSIST_FILTERS_COUNT 4
#define FORT_PROV_CALLOUT_FILTERS_COUNT 4
#define FORT_PROV_FLOW_FILTERS_COUNT    4
#define FORT_PROV_PACKET_FILTERS_COUNT  4
#define FORT_PROV_REAUTH_FILTERS_COUNT  4

static struct
{
    FORT_PROV_BOOT_CONF boot_conf;

    FWPM_PROVIDER0 provider;
    FWPM_PROVIDER0 boot_provider;

    FWPM_SUBLAYER0 sublayer;
    FWPM_SUBLAYER0 boot_sublayer;

    FWPM_CALLOUT0 callouts[FORT_PROV_CALLOUTS_COUNT];

    FWPM_FILTER0 boot_filters[FORT_PROV_BOOT_FILTERS_COUNT];
    FWPM_FILTER0 persist_filters[FORT_PROV_PERSIST_FILTERS_COUNT];

    FWPM_FILTER0 callout_filters[FORT_PROV_CALLOUT_FILTERS_COUNT];
    FWPM_FILTER0 callout_boot_filters[FORT_PROV_CALLOUT_FILTERS_COUNT];

    FWPM_FILTER0 flow_filters[FORT_PROV_FLOW_FILTERS_COUNT];
    FWPM_FILTER0 packet_filters[FORT_PROV_PACKET_FILTERS_COUNT];

    FWPM_FILTER0 reauth_filters[FORT_PROV_REAUTH_FILTERS_COUNT];
} g_provGlobal;

static void fort_prov_init_callout(
        FWPM_CALLOUT0 *cout, GUID key, PCWCH name, PCWCH descr, GUID layer)
{
    cout->calloutKey = key;
    cout->displayData.name = (PWCHAR) name;
    cout->displayData.description = (PWCHAR) descr;
    cout->providerKey = (GUID *) &FORT_GUID_PROVIDER;
    cout->applicableLayer = layer;
}

static void fort_prov_init_callouts(void)
{
    FWPM_CALLOUT0 *cout = g_provGlobal.callouts;

    /* ocallout4 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_CONNECT_V4, L"FortCalloutConnect4",
            L"Fort Firewall Callout Connect V4", FWPM_LAYER_ALE_AUTH_CONNECT_V4);
    /* ocallout6 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_CONNECT_V6, L"FortCalloutConnect6",
            L"Fort Firewall Callout Connect V6", FWPM_LAYER_ALE_AUTH_CONNECT_V6);
    /* icallout4 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_ACCEPT_V4, L"FortCalloutAccept4",
            L"Fort Firewall Callout Accept V4", FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4);
    /* icallout6 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_ACCEPT_V6, L"FortCalloutAccept6",
            L"Fort Firewall Callout Accept V6", FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6);
    /* scallout4 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_STREAM_V4, L"FortCalloutStream4",
            L"Fort Firewall Callout Stream V4", FWPM_LAYER_STREAM_V4);
    /* scallout6 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_STREAM_V6, L"FortCalloutStream6",
            L"Fort Firewall Callout Stream V6", FWPM_LAYER_STREAM_V6);
    /* dcallout4 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_DATAGRAM_V4, L"FortCalloutDatagram4",
            L"Fort Firewall Callout Datagram V4", FWPM_LAYER_DATAGRAM_DATA_V4);
    /* dcallout6 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_DATAGRAM_V6, L"FortCalloutDatagram6",
            L"Fort Firewall Callout Datagram V6", FWPM_LAYER_DATAGRAM_DATA_V6);
    /* itcallout4 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_V4, L"FortCalloutInTransport4",
            L"Fort Firewall Callout Inbound Transport V4", FWPM_LAYER_INBOUND_TRANSPORT_V4);
    /* itcallout6 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_V6, L"FortCalloutInTransport6",
            L"Fort Firewall Callout Inbound Transport V6", FWPM_LAYER_INBOUND_TRANSPORT_V6);
    /* otcallout4 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_OUT_TRANSPORT_V4, L"FortCalloutOutTransport4",
            L"Fort Firewall Callout Outbound Transport V4", FWPM_LAYER_OUTBOUND_TRANSPORT_V4);
    /* otcallout6 */
    fort_prov_init_callout(cout++, FORT_GUID_CALLOUT_OUT_TRANSPORT_V6, L"FortCalloutOutTransport6",
            L"Fort Firewall Callout Outbound Transport V6", FWPM_LAYER_OUTBOUND_TRANSPORT_V6);
}

static void fort_prov_init_filter(FWPM_FILTER0 *filter, GUID filterKey, GUID layerKey,
        GUID subLayerKey, PCWCH name, PCWCH descr, FWP_VALUE0 weight, UINT32 flags,
        FWP_ACTION_TYPE actionType)
{
    filter->flags = flags;
    filter->filterKey = filterKey;
    filter->layerKey = layerKey;
    filter->subLayerKey = subLayerKey;
    filter->weight = weight;
    filter->displayData.name = (PWCHAR) name;
    filter->displayData.description = (PWCHAR) descr;
    filter->action.type = actionType;
}

static void fort_prov_init_boot_filter(
        FWPM_FILTER0 *filter, GUID filterKey, GUID layerKey, PCWCH name)
{
    const FWP_VALUE0 weight = { 0 };

    const UINT32 flags = FWPM_FILTER_FLAG_BOOTTIME | FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT;

    fort_prov_init_filter(filter, filterKey, layerKey, FORT_GUID_EMPTY, name, NULL, weight, flags,
            FWP_ACTION_BLOCK);
}

static void fort_prov_init_boot_filters(void)
{
    FWPM_FILTER0 *filter = g_provGlobal.boot_filters;

    /* ofilter4 */
    fort_prov_init_boot_filter(filter++, FORT_GUID_BOOT_FILTER_CONNECT_V4,
            FWPM_LAYER_ALE_AUTH_CONNECT_V4, L"FortBootFilterOut4");
    /* ofilter6 */
    fort_prov_init_boot_filter(filter++, FORT_GUID_BOOT_FILTER_CONNECT_V6,
            FWPM_LAYER_ALE_AUTH_CONNECT_V6, L"FortBootFilterOut6");
    /* ifilter4 */
    fort_prov_init_boot_filter(filter++, FORT_GUID_BOOT_FILTER_ACCEPT_V4,
            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, L"FortBootFilterIn4");
    /* ifilter6 */
    fort_prov_init_boot_filter(filter++, FORT_GUID_BOOT_FILTER_ACCEPT_V6,
            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, L"FortBootFilterIn6");
}

static void fort_prov_init_persist_filter(
        FWPM_FILTER0 *filter, GUID filterKey, GUID layerKey, PCWCH name)
{
    FWP_VALUE0 weight;
    weight.type = FWP_UINT8;
    weight.uint8 = 0; /* low weight */

    const UINT32 flags = FWPM_FILTER_FLAG_PERSISTENT | FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT;

    fort_prov_init_filter(filter, filterKey, layerKey, FORT_GUID_SUBLAYER, name, NULL, weight,
            flags, FWP_ACTION_BLOCK);
}

static void fort_prov_init_persist_filters(void)
{
    FWPM_FILTER0 *filter = g_provGlobal.persist_filters;

    /* ofilter4 */
    fort_prov_init_persist_filter(filter++, FORT_GUID_PERSIST_FILTER_CONNECT_V4,
            FWPM_LAYER_ALE_AUTH_CONNECT_V4, L"FortPersistFilterConnect4");
    /* ofilter6 */
    fort_prov_init_persist_filter(filter++, FORT_GUID_PERSIST_FILTER_CONNECT_V6,
            FWPM_LAYER_ALE_AUTH_CONNECT_V6, L"FortPersistFilterConnect6");
    /* ifilter4 */
    fort_prov_init_persist_filter(filter++, FORT_GUID_PERSIST_FILTER_ACCEPT_V4,
            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, L"FortPersistFilterAccept4");
    /* ifilter6 */
    fort_prov_init_persist_filter(filter++, FORT_GUID_PERSIST_FILTER_ACCEPT_V6,
            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, L"FortPersistFilterAccept6");
}

static void fort_prov_init_callout_filter(FWPM_FILTER0 *filter, GUID filterKey, GUID layerKey,
        GUID calloutKey, PCWCH name, PCWCH descr)
{
    FWP_VALUE0 weight;
    weight.type = FWP_UINT8;
    weight.uint8 = 9; /* high weight */

    const UINT32 flags = FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED;

    fort_prov_init_filter(filter, filterKey, layerKey, FORT_GUID_SUBLAYER, name, descr, weight,
            flags, FWP_ACTION_CALLOUT_UNKNOWN);

    filter->action.calloutKey = calloutKey;
}

static void fort_prov_init_callout_filters(void)
{
    FWPM_FILTER0 *filter = g_provGlobal.callout_filters;

    /* ofilter4 */
    fort_prov_init_callout_filter(filter++, FORT_GUID_FILTER_CONNECT_V4,
            FWPM_LAYER_ALE_AUTH_CONNECT_V4, FORT_GUID_CALLOUT_CONNECT_V4, L"FortFilterConnect4",
            L"Fort Firewall Filter Connect V4");
    /* ofilter6 */
    fort_prov_init_callout_filter(filter++, FORT_GUID_FILTER_CONNECT_V6,
            FWPM_LAYER_ALE_AUTH_CONNECT_V6, FORT_GUID_CALLOUT_CONNECT_V6, L"FortFilterConnect6",
            L"Fort Firewall Filter Connect V6");
    /* ifilter4 */
    fort_prov_init_callout_filter(filter++, FORT_GUID_FILTER_ACCEPT_V4,
            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, FORT_GUID_CALLOUT_ACCEPT_V4, L"FortFilterAccept4",
            L"Fort Firewall Filter Accept V4");
    /* ifilter6 */
    fort_prov_init_callout_filter(filter++, FORT_GUID_FILTER_ACCEPT_V6,
            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, FORT_GUID_CALLOUT_ACCEPT_V6, L"FortFilterAccept6",
            L"Fort Firewall Filter Accept V6");
}

static void fort_prov_init_callout_boot_filters(void)
{
    FWPM_FILTER0 *filter = g_provGlobal.callout_filters;
    FWPM_FILTER0 *boot_filter = g_provGlobal.callout_boot_filters;

    for (int i = 0; i < FORT_PROV_CALLOUT_FILTERS_COUNT; ++i) {
        *boot_filter = *filter;
        boot_filter->flags = 0;

        ++filter;
        ++boot_filter;
    }
}

static void fort_prov_init_flow_filter(FWPM_FILTER0 *filter, GUID filterKey, GUID layerKey,
        GUID calloutKey, PCWCH name, PCWCH descr)
{
    const FWP_VALUE0 weight = { 0 };

    const UINT32 flags = (FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED
            | FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION);

    fort_prov_init_filter(filter, filterKey, layerKey, FORT_GUID_SUBLAYER, name, descr, weight,
            flags, FWP_ACTION_CALLOUT_TERMINATING);

    filter->action.calloutKey = calloutKey;
}

static void fort_prov_init_flow_filters(void)
{
    FWPM_FILTER0 *filter = g_provGlobal.flow_filters;

    /* sfilter4 */
    fort_prov_init_flow_filter(filter++, FORT_GUID_FILTER_STREAM_V4, FWPM_LAYER_STREAM_V4,
            FORT_GUID_CALLOUT_STREAM_V4, L"FortFilterStream4", L"Fort Firewall Filter Stream V4");
    /* sfilter6 */
    fort_prov_init_flow_filter(filter++, FORT_GUID_FILTER_STREAM_V6, FWPM_LAYER_STREAM_V6,
            FORT_GUID_CALLOUT_STREAM_V6, L"FortFilterStream6", L"Fort Firewall Filter Stream V6");
    /* dfilter4 */
    fort_prov_init_flow_filter(filter++, FORT_GUID_FILTER_DATAGRAM_V4, FWPM_LAYER_DATAGRAM_DATA_V4,
            FORT_GUID_CALLOUT_DATAGRAM_V4, L"FortFilterDatagram4",
            L"Fort Firewall Filter Datagram V4");
    /* dfilter6 */
    fort_prov_init_flow_filter(filter++, FORT_GUID_FILTER_DATAGRAM_V6, FWPM_LAYER_DATAGRAM_DATA_V6,
            FORT_GUID_CALLOUT_DATAGRAM_V6, L"FortFilterDatagram6",
            L"Fort Firewall Filter Datagram V6");
}

static void fort_prov_init_packet_filters(void)
{
    FWPM_FILTER0 *filter = g_provGlobal.packet_filters;

    /* itfilter4 */
    fort_prov_init_flow_filter(filter++, FORT_GUID_FILTER_IN_TRANSPORT_V4,
            FWPM_LAYER_INBOUND_TRANSPORT_V4, FORT_GUID_CALLOUT_IN_TRANSPORT_V4,
            L"FortFilterInTransport4", L"Fort Firewall Filter Inbound Transport V4");
    /* itfilter6 */
    fort_prov_init_flow_filter(filter++, FORT_GUID_FILTER_IN_TRANSPORT_V6,
            FWPM_LAYER_INBOUND_TRANSPORT_V6, FORT_GUID_CALLOUT_IN_TRANSPORT_V6,
            L"FortFilterInTransport6", L"Fort Firewall Filter Inbound Transport V6");
    /* otfilter4 */
    fort_prov_init_flow_filter(filter++, FORT_GUID_FILTER_OUT_TRANSPORT_V4,
            FWPM_LAYER_OUTBOUND_TRANSPORT_V4, FORT_GUID_CALLOUT_OUT_TRANSPORT_V4,
            L"FortFilterOutTransport4", L"Fort Firewall Filter Outbound Transport V4");
    /* otfilter6 */
    fort_prov_init_flow_filter(filter++, FORT_GUID_FILTER_OUT_TRANSPORT_V6,
            FWPM_LAYER_OUTBOUND_TRANSPORT_V6, FORT_GUID_CALLOUT_OUT_TRANSPORT_V6,
            L"FortFilterOutTransport6", L"Fort Firewall Filter Outbound Transport V6");
}

static void fort_prov_init_reauth_filter(
        FWPM_FILTER0 *filter, GUID filterKey, GUID layerKey, PCWCH name)
{
    const FWP_VALUE0 weight = { 0 };

    fort_prov_init_filter(filter, filterKey, layerKey, FORT_GUID_SUBLAYER, name, NULL, weight, 0,
            FWP_ACTION_CONTINUE);
}

static void fort_prov_init_reauth_filters(void)
{
    FWPM_FILTER0 *filter = g_provGlobal.reauth_filters;

    /* ifilter4 */
    fort_prov_init_reauth_filter(filter++, FORT_GUID_FILTER_REAUTH_IN_V4,
            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, L"FortFilterReauthIn4");
    /* ifilter6 */
    fort_prov_init_reauth_filter(filter++, FORT_GUID_FILTER_REAUTH_IN_V6,
            FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, L"FortFilterReauthIn6");
    /* ofilter4 */
    fort_prov_init_reauth_filter(filter++, FORT_GUID_FILTER_REAUTH_OUT_V4,
            FWPM_LAYER_ALE_AUTH_CONNECT_V4, L"FortFilterReauthOut4");
    /* ofilter6 */
    fort_prov_init_reauth_filter(filter++, FORT_GUID_FILTER_REAUTH_OUT_V6,
            FWPM_LAYER_ALE_AUTH_CONNECT_V6, L"FortFilterReauthOut6");
}

static void fort_prov_init_provider(void)
{
    FWPM_PROVIDER0 *provider = &g_provGlobal.provider;
    provider->flags = 0;
    provider->providerKey = FORT_GUID_PROVIDER;
    provider->displayData.name = (PWCHAR) L"FortProvider";
    provider->displayData.description = (PWCHAR) L"Fort Firewall Provider";
    provider->providerData.size = sizeof(FORT_PROV_BOOT_CONF);
    provider->providerData.data = (PUINT8) &g_provGlobal.boot_conf;
    provider->serviceName = (PWCHAR) L"fortfw";

    FWPM_PROVIDER0 *boot_provider = &g_provGlobal.boot_provider;
    *boot_provider = *provider;
    boot_provider->flags = FWPM_PROVIDER_FLAG_PERSISTENT;
}

static void fort_prov_init_sublayer(void)
{
    FWPM_SUBLAYER0 *sublayer = &g_provGlobal.sublayer;
    sublayer->flags = 0;
    sublayer->subLayerKey = FORT_GUID_SUBLAYER;
    sublayer->displayData.name = (PWCHAR) L"FortSublayer";
    sublayer->displayData.description = (PWCHAR) L"Fort Firewall Sublayer";
    sublayer->providerKey = (GUID *) &FORT_GUID_PROVIDER;

    FWPM_SUBLAYER0 *boot_sublayer = &g_provGlobal.boot_sublayer;
    *boot_sublayer = *sublayer;
    boot_sublayer->flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
}

FORT_API void fort_prov_init()
{
    RtlZeroMemory(&g_provGlobal, sizeof(g_provGlobal));

    fort_prov_init_provider();
    fort_prov_init_sublayer();

    fort_prov_init_callouts();

    fort_prov_init_boot_filters();
    fort_prov_init_persist_filters();

    fort_prov_init_callout_filters();
    fort_prov_init_callout_boot_filters();

    fort_prov_init_flow_filters();
    fort_prov_init_packet_filters();

    fort_prov_init_reauth_filters();
}

FORT_API DWORD fort_prov_trans_open(HANDLE *engine)
{
    DWORD status = fort_prov_open(engine);

    if (status == 0) {
        status = fort_prov_trans_begin(*engine);

        if (status != 0) {
            fort_prov_close(*engine);
        }
    }

    return status;
}

FORT_API DWORD fort_prov_trans_close(HANDLE engine, DWORD status)
{
    if (status == 0) {
        status = fort_prov_trans_commit(engine);
    } else {
        fort_prov_trans_abort(engine);
    }

    fort_prov_close(engine);

    return status;
}

static DWORD fort_prov_add_callouts(HANDLE engine, const FWPM_CALLOUT0 *callouts, int count)
{
    for (int i = 0; i < count; ++i) {
        const DWORD status = FwpmCalloutAdd0(engine, &callouts[i], NULL, NULL);
        if (status)
            return status;
    }

    return 0;
}

static DWORD fort_prov_add_filters(HANDLE engine, const FWPM_FILTER0 *filters, int count)
{
    for (int i = 0; i < count; ++i) {
        const DWORD status = FwpmFilterAdd0(engine, &filters[i], NULL, NULL);
        if (status)
            return status;
    }

    return 0;
}

static void fort_prov_unregister_filters(HANDLE engine)
{
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_BOOT_FILTER_CONNECT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_BOOT_FILTER_CONNECT_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_BOOT_FILTER_ACCEPT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_BOOT_FILTER_ACCEPT_V6);

    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_PERSIST_FILTER_CONNECT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_PERSIST_FILTER_CONNECT_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_PERSIST_FILTER_ACCEPT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_PERSIST_FILTER_ACCEPT_V6);

    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_CONNECT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_CONNECT_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_ACCEPT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_ACCEPT_V6);
}

static DWORD fort_prov_unregister_reauth_filters(HANDLE engine)
{
    const DWORD status = FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_IN_V4);

    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_IN_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_OUT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_OUT_V6);

    return status;
}

static void fort_prov_unregister_callouts(HANDLE engine)
{
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_CONNECT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_CONNECT_V6);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_ACCEPT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_ACCEPT_V6);

    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_STREAM_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_STREAM_V6);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_DATAGRAM_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_DATAGRAM_V6);

    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_TRANSPORT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_TRANSPORT_V6);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_OUT_TRANSPORT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_OUT_TRANSPORT_V6);
}

static void fort_prov_unregister_provider(HANDLE engine)
{
    fort_prov_unregister_filters(engine);
    fort_prov_unregister_reauth_filters(engine);

    fort_prov_unregister_callouts(engine);

    FwpmSubLayerDeleteByKey0(engine, (GUID *) &FORT_GUID_SUBLAYER);
    FwpmProviderDeleteByKey0(engine, (GUID *) &FORT_GUID_PROVIDER);
}

FORT_API void fort_prov_flow_unregister(HANDLE engine)
{
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_STREAM_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_STREAM_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_DATAGRAM_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_DATAGRAM_V6);

    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_OUT_TRANSPORT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_OUT_TRANSPORT_V6);
}

FORT_API void fort_prov_unregister(HANDLE engine)
{
    fort_prov_flow_unregister(engine);
    fort_prov_unregister_provider(engine);
}

FORT_API void fort_prov_trans_unregister(void)
{
    HANDLE engine;
    if (NT_SUCCESS(fort_prov_trans_open(&engine))) {

        fort_prov_unregister(engine);

        fort_prov_trans_close(engine, /*status=*/0);
    }
}

static DWORD fort_prov_register_filters(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    if (boot_conf.boot_filter) {
        DWORD status;

        if ((status = fort_prov_add_filters(
                     engine, g_provGlobal.boot_filters, FORT_PROV_BOOT_FILTERS_COUNT)))
            return status;

        if ((status = fort_prov_add_filters(
                     engine, g_provGlobal.persist_filters, FORT_PROV_PERSIST_FILTERS_COUNT)))
            return status;
    }

    return fort_prov_add_filters(engine,
            (boot_conf.boot_filter ? g_provGlobal.callout_boot_filters
                                   : g_provGlobal.callout_filters),
            FORT_PROV_CALLOUT_FILTERS_COUNT);
}

static DWORD fort_prov_register_provider(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    g_provGlobal.boot_conf = boot_conf;

    FWPM_PROVIDER0 *provider =
            boot_conf.boot_filter ? &g_provGlobal.boot_provider : &g_provGlobal.provider;

    return FwpmProviderAdd0(engine, provider, NULL);
}

static DWORD fort_prov_register_sublayer(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    FWPM_SUBLAYER0 *sublayer =
            boot_conf.boot_filter ? &g_provGlobal.boot_sublayer : &g_provGlobal.sublayer;

    return FwpmSubLayerAdd0(engine, sublayer, NULL);
}

FORT_API DWORD fort_prov_register(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    DWORD status;

    if ((status = fort_prov_register_provider(engine, boot_conf)))
        return status;

    if ((status = fort_prov_register_sublayer(engine, boot_conf)))
        return status;

    if ((status = fort_prov_add_callouts(engine, g_provGlobal.callouts, FORT_PROV_CALLOUTS_COUNT)))
        return status;

    if ((status = fort_prov_register_filters(engine, boot_conf)))
        return status;

    return 0;
}

FORT_API DWORD fort_prov_trans_register(const FORT_PROV_BOOT_CONF boot_conf)
{
    HANDLE engine;
    DWORD status = fort_prov_trans_open(&engine);

    if (NT_SUCCESS(status)) {

        status = fort_prov_register(engine, boot_conf);

        status = fort_prov_trans_close(engine, status);
    }

    return status;
}

FORT_API BOOL fort_prov_get_boot_conf(HANDLE engine, PFORT_PROV_BOOT_CONF boot_conf)
{
    FWPM_PROVIDER0 *provider;

    if (!FwpmProviderGetByKey0(engine, (GUID *) &FORT_GUID_PROVIDER, &provider)) {
        boot_conf->boot_filter = (provider->flags & FWPM_PROVIDER_FLAG_PERSISTENT);

        if (provider->providerData.size == sizeof(FORT_PROV_BOOT_CONF)) {
            RtlCopyMemory(boot_conf, provider->providerData.data, sizeof(FORT_PROV_BOOT_CONF));
        }

        FwpmFreeMemory0((void **) &provider);

        return TRUE;
    }

    return FALSE;
}

FORT_API DWORD fort_prov_flow_register(HANDLE engine, BOOL filter_packets)
{
    DWORD status;

    status = fort_prov_add_filters(engine, g_provGlobal.flow_filters, FORT_PROV_FLOW_FILTERS_COUNT);

    if (status == 0 && filter_packets) {
        status = fort_prov_add_filters(
                engine, g_provGlobal.packet_filters, FORT_PROV_PACKET_FILTERS_COUNT);
    }

    return status;
}

FORT_API void fort_prov_reauth(HANDLE engine)
{
    if (fort_prov_unregister_reauth_filters(engine) != 0) {
        fort_prov_add_filters(engine, g_provGlobal.reauth_filters, FORT_PROV_REAUTH_FILTERS_COUNT);
    }
}
