/* Fort Firewall Driver Provider (Un)Registration */

#include "fortprov.h"

#include "fortioctl.h"

#define FORT_PROV_BOOT_FILTERS_COUNT    4
#define FORT_PROV_PERSIST_FILTERS_COUNT 4
#define FORT_PROV_CALLOUT_FILTERS_COUNT 4
#define FORT_PROV_PACKET_FILTERS_COUNT  4
#define FORT_PROV_DISCARD_FILTERS_COUNT 4
#define FORT_PROV_REAUTH_FILTERS_COUNT  4

#define FORT_PROV_CALLOUTS_COUNT                                                                   \
    (FORT_PROV_CALLOUT_FILTERS_COUNT + FORT_PROV_PACKET_FILTERS_COUNT                              \
            + FORT_PROV_DISCARD_FILTERS_COUNT)

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

    FWPM_FILTER0 packet_filters[FORT_PROV_PACKET_FILTERS_COUNT];
    FWPM_FILTER0 discard_filters[FORT_PROV_DISCARD_FILTERS_COUNT];

    FWPM_FILTER0 reauth_filters[FORT_PROV_REAUTH_FILTERS_COUNT];
} g_provGlobal;

typedef struct fort_prov_init_callout_args
{
    GUID key;
    PCWCH name;
    PCWCH descr;
    GUID layer;
} FORT_PROV_INIT_CALLOUT_ARGS;

static void fort_prov_init_callout(FWPM_CALLOUT0 *cout, FORT_PROV_INIT_CALLOUT_ARGS ica)
{
    cout->calloutKey = ica.key;
    cout->displayData.name = (PWCHAR) ica.name;
    cout->displayData.description = (PWCHAR) ica.descr;
    cout->providerKey = (GUID *) &FORT_GUID_PROVIDER;
    cout->applicableLayer = ica.layer;
}

static void fort_prov_init_callouts(void)
{
    const FORT_PROV_INIT_CALLOUT_ARGS args[] = {
        /* ocallout4 */
        { FORT_GUID_CALLOUT_CONNECT_V4, L"FortCalloutConnect4", L"Fort Firewall Callout Connect V4",
                FWPM_LAYER_ALE_AUTH_CONNECT_V4 },
        /* ocallout6 */
        { FORT_GUID_CALLOUT_CONNECT_V6, L"FortCalloutConnect6", L"Fort Firewall Callout Connect V6",
                FWPM_LAYER_ALE_AUTH_CONNECT_V6 },
        /* icallout4 */
        { FORT_GUID_CALLOUT_ACCEPT_V4, L"FortCalloutAccept4", L"Fort Firewall Callout Accept V4",
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4 },
        /* icallout6 */
        { FORT_GUID_CALLOUT_ACCEPT_V6, L"FortCalloutAccept6", L"Fort Firewall Callout Accept V6",
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6 },
        /* itcallout4 */
        { FORT_GUID_CALLOUT_IN_TRANSPORT_V4, L"FortCalloutInTransport4",
                L"Fort Firewall Callout Inbound Transport V4", FWPM_LAYER_INBOUND_TRANSPORT_V4 },
        /* itcallout6 */
        { FORT_GUID_CALLOUT_IN_TRANSPORT_V6, L"FortCalloutInTransport6",
                L"Fort Firewall Callout Inbound Transport V6", FWPM_LAYER_INBOUND_TRANSPORT_V6 },
        /* otcallout4 */
        { FORT_GUID_CALLOUT_OUT_TRANSPORT_V4, L"FortCalloutOutTransport4",
                L"Fort Firewall Callout Outbound Transport V4", FWPM_LAYER_OUTBOUND_TRANSPORT_V4 },
        /* otcallout6 */
        { FORT_GUID_CALLOUT_OUT_TRANSPORT_V6, L"FortCalloutOutTransport6",
                L"Fort Firewall Callout Outbound Transport V6", FWPM_LAYER_OUTBOUND_TRANSPORT_V6 },
        /* itdcallout4 */
        { FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V4, L"FortCalloutInTransportDiscard4",
                L"Fort Firewall Callout Inbound Transport Discard V4",
                FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD },
        /* itdcallout6 */
        { FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V6, L"FortCalloutInTransportDiscard6",
                L"Fort Firewall Callout Inbound Transport Discard V6",
                FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD },
        /* ipdcallout4 */
        { FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V4, L"FortCalloutInIpPacketDiscard4",
                L"Fort Firewall Callout Inbound IpPacket Discard V4",
                FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD },
        /* ipdcallout6 */
        { FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V6, L"FortCalloutInIpPacketDiscard6",
                L"Fort Firewall Callout Inbound IpPacket Discard V6",
                FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD },
    };

    FWPM_CALLOUT0 *cout = g_provGlobal.callouts;

    for (int i = 0; i < FORT_PROV_CALLOUTS_COUNT; ++i) {
        fort_prov_init_callout(cout++, args[i]);
    }
}

typedef struct fort_prov_init_filter_args
{
    GUID filterKey;
    GUID layerKey;
    GUID subLayerKey;
    PCWCH name;
    PCWCH descr;
    FWP_VALUE0 weight;
    UINT32 flags;
    FWP_ACTION_TYPE actionType;
    GUID calloutKey;
} FORT_PROV_INIT_FILTER_ARGS, *PFORT_PROV_INIT_FILTER_ARGS;

typedef const FORT_PROV_INIT_FILTER_ARGS *PCFORT_PROV_INIT_FILTER_ARGS;

static void fort_prov_init_filter(FWPM_FILTER0 *filter, PCFORT_PROV_INIT_FILTER_ARGS ifa)
{
    filter->flags = ifa->flags;
    filter->filterKey = ifa->filterKey;
    filter->layerKey = ifa->layerKey;
    filter->subLayerKey = ifa->subLayerKey;
    filter->weight = ifa->weight;
    filter->displayData.name = (PWCHAR) ifa->name;
    filter->displayData.description = (PWCHAR) ifa->descr;
    filter->action.type = ifa->actionType;
    filter->action.calloutKey = ifa->calloutKey;
}

static void fort_prov_init_filters(
        FWPM_FILTER0 *filter, PCFORT_PROV_INIT_FILTER_ARGS args, int count)
{
    for (int i = 0; i < count; ++i) {
        fort_prov_init_filter(filter++, &args[i]);
    }
}

static void fort_prov_init_boot_filters(void)
{
    const FORT_PROV_INIT_FILTER_ARGS d = {
        .subLayerKey = FORT_GUID_EMPTY,
        .flags = FWPM_FILTER_FLAG_BOOTTIME | FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT,
        .actionType = FWP_ACTION_BLOCK,
    };

    const FORT_PROV_INIT_FILTER_ARGS args[] = {
        /* ofilter4 */
        { FORT_GUID_BOOT_FILTER_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V4, d.subLayerKey,
                L"FortBootFilterOut4", d.descr, d.weight, d.flags, d.actionType },
        /* ofilter6 */
        { FORT_GUID_BOOT_FILTER_CONNECT_V6, FWPM_LAYER_ALE_AUTH_CONNECT_V6, d.subLayerKey,
                L"FortBootFilterOut6", d.descr, d.weight, d.flags, d.actionType },
        /* ifilter4 */
        { FORT_GUID_BOOT_FILTER_ACCEPT_V4, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, d.subLayerKey,
                L"FortBootFilterIn4", d.descr, d.weight, d.flags, d.actionType },
        /* ifilter6 */
        { FORT_GUID_BOOT_FILTER_ACCEPT_V6, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, d.subLayerKey,
                L"FortBootFilterIn6", d.descr, d.weight, d.flags, d.actionType },
    };

    fort_prov_init_filters(g_provGlobal.boot_filters, args, FORT_PROV_BOOT_FILTERS_COUNT);
}

static void fort_prov_init_persist_filters(void)
{
    const FORT_PROV_INIT_FILTER_ARGS d = {
        .subLayerKey = FORT_GUID_SUBLAYER,
        /* low weight */
        .flags = FWPM_FILTER_FLAG_PERSISTENT | FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT,
        .actionType = FWP_ACTION_BLOCK,
    };

    const FORT_PROV_INIT_FILTER_ARGS args[] = {
        /* ofilter4 */
        { FORT_GUID_PERSIST_FILTER_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V4, d.subLayerKey,
                L"FortPersistFilterConnect4", d.descr, d.weight, d.flags, d.actionType },
        /* ofilter6 */
        { FORT_GUID_PERSIST_FILTER_CONNECT_V6, FWPM_LAYER_ALE_AUTH_CONNECT_V6, d.subLayerKey,
                L"FortPersistFilterConnect6", d.descr, d.weight, d.flags, d.actionType },
        /* ifilter4 */
        { FORT_GUID_PERSIST_FILTER_ACCEPT_V4, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, d.subLayerKey,
                L"FortPersistFilterAccept4", d.descr, d.weight, d.flags, d.actionType },
        /* ifilter6 */
        { FORT_GUID_PERSIST_FILTER_ACCEPT_V6, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, d.subLayerKey,
                L"FortPersistFilterAccept6", d.descr, d.weight, d.flags, d.actionType },
    };

    fort_prov_init_filters(g_provGlobal.persist_filters, args, FORT_PROV_PERSIST_FILTERS_COUNT);
}

static void fort_prov_init_callout_filters(void)
{
    const FWP_VALUE0 high_weight = { .type = FWP_UINT8, .uint8 = 9 };

    const FORT_PROV_INIT_FILTER_ARGS d = {
        .subLayerKey = FORT_GUID_SUBLAYER,
        .weight = high_weight,
        .flags = FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED,
        .actionType = FWP_ACTION_CALLOUT_UNKNOWN,
    };

    const FORT_PROV_INIT_FILTER_ARGS args[] = {
        /* ofilter4 */
        { FORT_GUID_FILTER_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V4, d.subLayerKey,
                L"FortFilterConnect4", L"Fort Firewall Filter Connect V4", d.weight, d.flags,
                d.actionType, FORT_GUID_CALLOUT_CONNECT_V4 },
        /* ofilter6 */
        { FORT_GUID_FILTER_CONNECT_V6, FWPM_LAYER_ALE_AUTH_CONNECT_V6, d.subLayerKey,
                L"FortFilterConnect6", L"Fort Firewall Filter Connect V6", d.weight, d.flags,
                d.actionType, FORT_GUID_CALLOUT_CONNECT_V6 },
        /* ifilter4 */
        { FORT_GUID_FILTER_ACCEPT_V4, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, d.subLayerKey,
                L"FortFilterAccept4", L"Fort Firewall Filter Accept V4", d.weight, d.flags,
                d.actionType, FORT_GUID_CALLOUT_ACCEPT_V4 },
        /* ifilter6 */
        { FORT_GUID_FILTER_ACCEPT_V6, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, d.subLayerKey,
                L"FortFilterAccept6", L"Fort Firewall Filter Accept V6", d.weight, d.flags,
                d.actionType, FORT_GUID_CALLOUT_ACCEPT_V6 },
    };

    fort_prov_init_filters(g_provGlobal.callout_filters, args, FORT_PROV_CALLOUT_FILTERS_COUNT);
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

static void fort_prov_init_packet_filters(void)
{
    const FORT_PROV_INIT_FILTER_ARGS d = {
        .subLayerKey = FORT_GUID_SUBLAYER,
        .flags = FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED
                | FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION,
        .actionType = FWP_ACTION_CALLOUT_UNKNOWN,
    };

    const FORT_PROV_INIT_FILTER_ARGS args[] = {
        /* itfilter4 */
        { FORT_GUID_FILTER_IN_TRANSPORT_V4, FWPM_LAYER_INBOUND_TRANSPORT_V4, d.subLayerKey,
                L"FortFilterInTransport4", L"Fort Firewall Filter Inbound Transport V4", d.weight,
                d.flags, d.actionType, FORT_GUID_CALLOUT_IN_TRANSPORT_V4 },
        /* itfilter6 */
        { FORT_GUID_FILTER_IN_TRANSPORT_V6, FWPM_LAYER_INBOUND_TRANSPORT_V6, d.subLayerKey,
                L"FortFilterInTransport6", L"Fort Firewall Filter Inbound Transport V6", d.weight,
                d.flags, d.actionType, FORT_GUID_CALLOUT_IN_TRANSPORT_V6 },
        /* otfilter4 */
        { FORT_GUID_FILTER_OUT_TRANSPORT_V4, FWPM_LAYER_OUTBOUND_TRANSPORT_V4, d.subLayerKey,
                L"FortFilterOutTransport4", L"Fort Firewall Filter Outbound Transport V4", d.weight,
                d.flags, d.actionType, FORT_GUID_CALLOUT_OUT_TRANSPORT_V4 },
        /* otfilter6 */
        { FORT_GUID_FILTER_OUT_TRANSPORT_V6, FWPM_LAYER_OUTBOUND_TRANSPORT_V6, d.subLayerKey,
                L"FortFilterOutTransport6", L"Fort Firewall Filter Outbound Transport V6", d.weight,
                d.flags, d.actionType, FORT_GUID_CALLOUT_OUT_TRANSPORT_V6 },
    };

    fort_prov_init_filters(g_provGlobal.packet_filters, args, FORT_PROV_PACKET_FILTERS_COUNT);
}

static void fort_prov_init_discard_filters(void)
{
    const FORT_PROV_INIT_FILTER_ARGS d = {
        .subLayerKey = FORT_GUID_SUBLAYER,
        .flags = FWPM_FILTER_FLAG_PERSISTENT,
        .actionType = FWP_ACTION_CALLOUT_TERMINATING,
    };

    const FORT_PROV_INIT_FILTER_ARGS args[] = {
        /* itdfilter4 */
        { FORT_GUID_FILTER_IN_TRANSPORT_DISCARD_V4, FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD,
                d.subLayerKey, L"FortFilterInTransportDiscard4",
                L"Fort Firewall Filter Inbound Transport Discard V4", d.weight, d.flags,
                d.actionType, FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V4 },
        /* itdfilter6 */
        { FORT_GUID_FILTER_IN_TRANSPORT_DISCARD_V6, FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD,
                d.subLayerKey, L"FortFilterInTransportDiscard6",
                L"Fort Firewall Filter Inbound Transport Discard V6", d.weight, d.flags,
                d.actionType, FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V6 },
        /* ipdfilter4 */
        { FORT_GUID_FILTER_IN_IPPACKET_DISCARD_V4, FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD,
                d.subLayerKey, L"FortFilterInIpPacketDiscard4",
                L"Fort Firewall Filter Inbound IpPacket Discard V4", d.weight, d.flags,
                d.actionType, FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V4 },
        /* ipdfilter6 */
        { FORT_GUID_FILTER_IN_IPPACKET_DISCARD_V6, FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD,
                d.subLayerKey, L"FortFilterInIpPacketDiscard6",
                L"Fort Firewall Filter Inbound IpPacket Discard V6", d.weight, d.flags,
                d.actionType, FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V6 },
    };

    fort_prov_init_filters(g_provGlobal.discard_filters, args, FORT_PROV_DISCARD_FILTERS_COUNT);
}

static void fort_prov_init_reauth_filters(void)
{
    const FORT_PROV_INIT_FILTER_ARGS d = {
        .subLayerKey = FORT_GUID_SUBLAYER,
        .actionType = FWP_ACTION_CONTINUE,
    };

    const FORT_PROV_INIT_FILTER_ARGS args[] = {
        /* ifilter4 */
        { FORT_GUID_FILTER_REAUTH_IN_V4, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, d.subLayerKey,
                L"FortFilterReauthIn4", d.descr, d.weight, d.flags, d.actionType },
        /* ifilter6 */
        { FORT_GUID_FILTER_REAUTH_IN_V6, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, d.subLayerKey,
                L"FortFilterReauthIn6", d.descr, d.weight, d.flags, d.actionType },
        /* ofilter4 */
        { FORT_GUID_FILTER_REAUTH_OUT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V4, d.subLayerKey,
                L"FortFilterReauthOut4", d.descr, d.weight, d.flags, d.actionType },
        /* ofilter6 */
        { FORT_GUID_FILTER_REAUTH_OUT_V6, FWPM_LAYER_ALE_AUTH_CONNECT_V6, d.subLayerKey,
                L"FortFilterReauthOut6", d.descr, d.weight, d.flags, d.actionType },
    };

    fort_prov_init_filters(g_provGlobal.reauth_filters, args, FORT_PROV_REAUTH_FILTERS_COUNT);
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

static void fort_prov_init_sublayer(const FORT_PROV_INIT_CONF init_conf)
{
    FWPM_SUBLAYER0 *sublayer = &g_provGlobal.sublayer;
    sublayer->flags = 0;
    sublayer->subLayerKey = FORT_GUID_SUBLAYER;
    sublayer->displayData.name = (PWCHAR) L"FortSublayer";
    sublayer->displayData.description = (PWCHAR) L"Fort Firewall Sublayer";
    sublayer->providerKey = (GUID *) &FORT_GUID_PROVIDER;
    sublayer->weight = init_conf.sublayer_weight;

    FWPM_SUBLAYER0 *boot_sublayer = &g_provGlobal.boot_sublayer;
    *boot_sublayer = *sublayer;
    boot_sublayer->flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
}

FORT_API void fort_prov_init(const FORT_PROV_INIT_CONF init_conf)
{
    RtlZeroMemory(&g_provGlobal, sizeof(g_provGlobal));

    fort_prov_init_provider();
    fort_prov_init_sublayer(init_conf);

    fort_prov_init_callouts();

    fort_prov_init_boot_filters();
    fort_prov_init_persist_filters();

    fort_prov_init_callout_filters();
    fort_prov_init_callout_boot_filters();

    fort_prov_init_packet_filters();
    fort_prov_init_discard_filters();

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

    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_TRANSPORT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_TRANSPORT_V6);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_OUT_TRANSPORT_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_OUT_TRANSPORT_V6);

    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V6);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V6);

    // TODO: COMPAT: Remove after v4.1.0 (via v4.0.0)
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_STREAM_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_STREAM_V6);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_DATAGRAM_V4);
    FwpmCalloutDeleteByKey0(engine, (GUID *) &FORT_GUID_CALLOUT_DATAGRAM_V6);
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
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_OUT_TRANSPORT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_OUT_TRANSPORT_V6);

    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_DISCARD_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_DISCARD_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_IPPACKET_DISCARD_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_IPPACKET_DISCARD_V6);

    // TODO: COMPAT: Remove after v4.1.0 (via v4.0.0)
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_STREAM_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_STREAM_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_DATAGRAM_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_DATAGRAM_V6);
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

    if (boot_conf.stealth_mode) {
        DWORD status;

        if ((status = fort_prov_add_filters(
                     engine, g_provGlobal.discard_filters, FORT_PROV_DISCARD_FILTERS_COUNT)))
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

FORT_API DWORD fort_prov_flow_register(HANDLE engine)
{
    return fort_prov_add_filters(
            engine, g_provGlobal.packet_filters, FORT_PROV_PACKET_FILTERS_COUNT);
}

FORT_API void fort_prov_reauth(HANDLE engine)
{
    if (fort_prov_unregister_reauth_filters(engine) != 0) {
        fort_prov_add_filters(engine, g_provGlobal.reauth_filters, FORT_PROV_REAUTH_FILTERS_COUNT);
    }
}
