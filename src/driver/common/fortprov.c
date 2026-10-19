/* Fort Firewall Driver Provider (Un)Registration */

#include "fortprov.h"

#include "fortguid.h"

#if defined(FORT_DRIVER)
#    include "../forttrace.h"
#else
#    define TRACE(event_code, status, error_value, sequence)
#endif

/* A group's objects: e.g. Connect V4, V6 and Accept V4, V6 */
#define FORT_PROV_GROUP_SIZE 4

static struct
{
    FORT_PROV_BOOT_CONF boot_conf;

    FWPM_PROVIDER0 provider;
    FWPM_PROVIDER0 boot_provider;

    FWPM_SUBLAYER0 sublayer;
    FWPM_SUBLAYER0 boot_sublayer;
} g_provGlobal;

typedef struct fort_prov_object_args
{
    const GUID *key;
    const GUID *layerKey;
    PCWCH name;
    PCWCH descr;
    const GUID *calloutKey;
} FORT_PROV_OBJECT_ARGS, *PFORT_PROV_OBJECT_ARGS;

typedef const FORT_PROV_OBJECT_ARGS *PCFORT_PROV_OBJECT_ARGS;

typedef struct fort_prov_group_args FORT_PROV_GROUP_ARGS, *PFORT_PROV_GROUP_ARGS;

typedef const FORT_PROV_GROUP_ARGS *PCFORT_PROV_GROUP_ARGS;

typedef DWORD(FORT_PROV_ADD_FUNC)(
        HANDLE engine, PCFORT_PROV_GROUP_ARGS ga, PCFORT_PROV_OBJECT_ARGS oa);

/* The objects of a type with their common fields */
struct fort_prov_group_args
{
    UINT32 flags;
    FWP_ACTION_TYPE actionType;
    FORT_PROV_ADD_FUNC *add_func;
    const GUID *subLayerKey;
    FWP_VALUE0 weight;
    FORT_PROV_OBJECT_ARGS objects[FORT_PROV_GROUP_SIZE];
};

static DWORD fort_prov_add_status(DWORD status, const FWPM_DISPLAY_DATA0 *displayData, GUID key)
{
    if (status) {
        LOG("Prov Add: %ws: Error: %x\n", displayData->name, status);
        /* The object's key (its first part) identifies the object in the event */
        TRACE(FORT_PROV_ADD_ERROR, status, key.Data1, 0);
    }

    return status;
}

inline static GUID fort_prov_guid(const GUID *guid)
{
    return (guid != NULL) ? *guid : FORT_GUID_EMPTY;
}

static DWORD fort_prov_add_callout(
        HANDLE engine, PCFORT_PROV_GROUP_ARGS ga, PCFORT_PROV_OBJECT_ARGS oa)
{
    UNUSED(ga);

    const FWPM_CALLOUT0 callout = {
        .calloutKey = *oa->key,
        .displayData = { .name = (PWCHAR) oa->name, .description = (PWCHAR) oa->descr },
        .providerKey = (GUID *) &FORT_GUID_PROVIDER,
        .applicableLayer = *oa->layerKey,
    };

    return fort_prov_add_status(FwpmCalloutAdd0(engine, &callout, NULL, NULL), &callout.displayData,
            callout.calloutKey);
}

static DWORD fort_prov_add_filter(
        HANDLE engine, PCFORT_PROV_GROUP_ARGS ga, PCFORT_PROV_OBJECT_ARGS oa)
{
    FWPM_FILTER0 filter = {
        .filterKey = *oa->key,
        .displayData = { .name = (PWCHAR) oa->name, .description = (PWCHAR) oa->descr },
        .flags = ga->flags,
        .layerKey = *oa->layerKey,
        .subLayerKey = fort_prov_guid(ga->subLayerKey),
        .weight = ga->weight,
        .action.type = ga->actionType,
    };

    filter.action.calloutKey = fort_prov_guid(oa->calloutKey);

    return fort_prov_add_status(
            FwpmFilterAdd0(engine, &filter, NULL, NULL), &filter.displayData, filter.filterKey);
}

static DWORD fort_prov_add_group(HANDLE engine, PCFORT_PROV_GROUP_ARGS ga)
{
    for (int i = 0; i < FORT_PROV_GROUP_SIZE; ++i) {
        const DWORD status = ga->add_func(engine, ga, &ga->objects[i]);
        if (status)
            return status;
    }

    return 0;
}

static const FORT_PROV_GROUP_ARGS g_provAleCallouts = {
    .add_func = &fort_prov_add_callout,
    .objects = {
        /* ocallout4 */
        { &FORT_GUID_CALLOUT_CONNECT_V4, &FWPM_LAYER_ALE_AUTH_CONNECT_V4, L"FortCalloutConnect4",
                L"Fort Firewall Callout Connect V4" },
        /* ocallout6 */
        { &FORT_GUID_CALLOUT_CONNECT_V6, &FWPM_LAYER_ALE_AUTH_CONNECT_V6, L"FortCalloutConnect6",
                L"Fort Firewall Callout Connect V6" },
        /* icallout4 */
        { &FORT_GUID_CALLOUT_ACCEPT_V4, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, L"FortCalloutAccept4",
                L"Fort Firewall Callout Accept V4" },
        /* icallout6 */
        { &FORT_GUID_CALLOUT_ACCEPT_V6, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, L"FortCalloutAccept6",
                L"Fort Firewall Callout Accept V6" },
    },
};

static const FORT_PROV_GROUP_ARGS g_provPacketCallouts = {
    .add_func = &fort_prov_add_callout,
    .objects = {
        /* itcallout4 */
        { &FORT_GUID_CALLOUT_IN_TRANSPORT_V4, &FWPM_LAYER_INBOUND_TRANSPORT_V4,
                L"FortCalloutInTransport4", L"Fort Firewall Callout Inbound Transport V4" },
        /* itcallout6 */
        { &FORT_GUID_CALLOUT_IN_TRANSPORT_V6, &FWPM_LAYER_INBOUND_TRANSPORT_V6,
                L"FortCalloutInTransport6", L"Fort Firewall Callout Inbound Transport V6" },
        /* otcallout4 */
        { &FORT_GUID_CALLOUT_OUT_TRANSPORT_V4, &FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
                L"FortCalloutOutTransport4", L"Fort Firewall Callout Outbound Transport V4" },
        /* otcallout6 */
        { &FORT_GUID_CALLOUT_OUT_TRANSPORT_V6, &FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
                L"FortCalloutOutTransport6", L"Fort Firewall Callout Outbound Transport V6" },
    },
};

static const FORT_PROV_GROUP_ARGS g_provDiscardCallouts = {
    .add_func = &fort_prov_add_callout,
    .objects = {
        /* itdcallout4 */
        { &FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V4, &FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD,
                L"FortCalloutInTransportDiscard4",
                L"Fort Firewall Callout Inbound Transport Discard V4" },
        /* itdcallout6 */
        { &FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V6, &FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD,
                L"FortCalloutInTransportDiscard6",
                L"Fort Firewall Callout Inbound Transport Discard V6" },
        /* ipdcallout4 */
        { &FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V4, &FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD,
                L"FortCalloutInIpPacketDiscard4",
                L"Fort Firewall Callout Inbound IpPacket Discard V4" },
        /* ipdcallout6 */
        { &FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V6, &FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD,
                L"FortCalloutInIpPacketDiscard6",
                L"Fort Firewall Callout Inbound IpPacket Discard V6" },
    },
};

static const FORT_PROV_GROUP_ARGS g_provBootFilters = {
    .flags = FWPM_FILTER_FLAG_BOOTTIME | FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT,
    .actionType = FWP_ACTION_BLOCK,
    .add_func = &fort_prov_add_filter,
    .objects = {
        /* ofilter4 */
        { &FORT_GUID_BOOT_FILTER_CONNECT_V4, &FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                L"FortBootFilterConnect4" },
        /* ofilter6 */
        { &FORT_GUID_BOOT_FILTER_CONNECT_V6, &FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                L"FortBootFilterConnect6" },
        /* ifilter4 */
        { &FORT_GUID_BOOT_FILTER_ACCEPT_V4, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                L"FortBootFilterAccept4" },
        /* ifilter6 */
        { &FORT_GUID_BOOT_FILTER_ACCEPT_V6, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                L"FortBootFilterAccept6" },
    },
};

static const FORT_PROV_GROUP_ARGS g_provPersistFilters = {
    /* low weight */
    .flags = FWPM_FILTER_FLAG_PERSISTENT | FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT,
    .actionType = FWP_ACTION_BLOCK,
    .add_func = &fort_prov_add_filter,
    .subLayerKey = &FORT_GUID_SUBLAYER,
    .objects = {
        /* ofilter4 */
        { &FORT_GUID_PERSIST_FILTER_CONNECT_V4, &FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                L"FortPersistFilterConnect4" },
        /* ofilter6 */
        { &FORT_GUID_PERSIST_FILTER_CONNECT_V6, &FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                L"FortPersistFilterConnect6" },
        /* ifilter4 */
        { &FORT_GUID_PERSIST_FILTER_ACCEPT_V4, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                L"FortPersistFilterAccept4" },
        /* ifilter6 */
        { &FORT_GUID_PERSIST_FILTER_ACCEPT_V6, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                L"FortPersistFilterAccept6" },
    },
};

static const FORT_PROV_GROUP_ARGS g_provCalloutFilters = {
    .flags = FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED,
    .actionType = FWP_ACTION_CALLOUT_UNKNOWN,
    .add_func = &fort_prov_add_filter,
    .subLayerKey = &FORT_GUID_SUBLAYER,
    /* high weight */
    .weight = { .type = FWP_UINT8, .uint8 = 9 },
    .objects = {
        /* ofilter4 */
        { &FORT_GUID_FILTER_CONNECT_V4, &FWPM_LAYER_ALE_AUTH_CONNECT_V4, L"FortFilterConnect4",
                L"Fort Firewall Filter Connect V4", &FORT_GUID_CALLOUT_CONNECT_V4 },
        /* ofilter6 */
        { &FORT_GUID_FILTER_CONNECT_V6, &FWPM_LAYER_ALE_AUTH_CONNECT_V6, L"FortFilterConnect6",
                L"Fort Firewall Filter Connect V6", &FORT_GUID_CALLOUT_CONNECT_V6 },
        /* ifilter4 */
        { &FORT_GUID_FILTER_ACCEPT_V4, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, L"FortFilterAccept4",
                L"Fort Firewall Filter Accept V4", &FORT_GUID_CALLOUT_ACCEPT_V4 },
        /* ifilter6 */
        { &FORT_GUID_FILTER_ACCEPT_V6, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6, L"FortFilterAccept6",
                L"Fort Firewall Filter Accept V6", &FORT_GUID_CALLOUT_ACCEPT_V6 },
    },
};

static const FORT_PROV_GROUP_ARGS g_provPacketFilters = {
    .flags = FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED,
    .actionType = FWP_ACTION_CALLOUT_UNKNOWN,
    .add_func = &fort_prov_add_filter,
    .subLayerKey = &FORT_GUID_SUBLAYER,
    .objects = {
        /* itfilter4 */
        { &FORT_GUID_FILTER_IN_TRANSPORT_V4, &FWPM_LAYER_INBOUND_TRANSPORT_V4,
                L"FortFilterInTransport4", L"Fort Firewall Filter Inbound Transport V4",
                &FORT_GUID_CALLOUT_IN_TRANSPORT_V4 },
        /* itfilter6 */
        { &FORT_GUID_FILTER_IN_TRANSPORT_V6, &FWPM_LAYER_INBOUND_TRANSPORT_V6,
                L"FortFilterInTransport6", L"Fort Firewall Filter Inbound Transport V6",
                &FORT_GUID_CALLOUT_IN_TRANSPORT_V6 },
        /* otfilter4 */
        { &FORT_GUID_FILTER_OUT_TRANSPORT_V4, &FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
                L"FortFilterOutTransport4", L"Fort Firewall Filter Outbound Transport V4",
                &FORT_GUID_CALLOUT_OUT_TRANSPORT_V4 },
        /* otfilter6 */
        { &FORT_GUID_FILTER_OUT_TRANSPORT_V6, &FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
                L"FortFilterOutTransport6", L"Fort Firewall Filter Outbound Transport V6",
                &FORT_GUID_CALLOUT_OUT_TRANSPORT_V6 },
    },
};

static const FORT_PROV_GROUP_ARGS g_provDiscardFilters = {
    .actionType = FWP_ACTION_CALLOUT_UNKNOWN,
    .add_func = &fort_prov_add_filter,
    .subLayerKey = &FORT_GUID_SUBLAYER,
    .objects = {
        /* itdfilter4 */
        { &FORT_GUID_FILTER_IN_TRANSPORT_DISCARD_V4, &FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD,
                L"FortFilterInTransportDiscard4",
                L"Fort Firewall Filter Inbound Transport Discard V4",
                &FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V4 },
        /* itdfilter6 */
        { &FORT_GUID_FILTER_IN_TRANSPORT_DISCARD_V6, &FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD,
                L"FortFilterInTransportDiscard6",
                L"Fort Firewall Filter Inbound Transport Discard V6",
                &FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V6 },
        /* ipdfilter4 */
        { &FORT_GUID_FILTER_IN_IPPACKET_DISCARD_V4, &FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD,
                L"FortFilterInIpPacketDiscard4",
                L"Fort Firewall Filter Inbound IpPacket Discard V4",
                &FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V4 },
        /* ipdfilter6 */
        { &FORT_GUID_FILTER_IN_IPPACKET_DISCARD_V6, &FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD,
                L"FortFilterInIpPacketDiscard6",
                L"Fort Firewall Filter Inbound IpPacket Discard V6",
                &FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V6 },
    },
};

static const FORT_PROV_GROUP_ARGS g_provReauthFilters = {
    .actionType = FWP_ACTION_CONTINUE,
    .add_func = &fort_prov_add_filter,
    .subLayerKey = &FORT_GUID_SUBLAYER,
    .objects = {
        /* ifilter4 */
        { &FORT_GUID_FILTER_REAUTH_IN_V4, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                L"FortFilterReauthIn4" },
        /* ifilter6 */
        { &FORT_GUID_FILTER_REAUTH_IN_V6, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                L"FortFilterReauthIn6" },
        /* ofilter4 */
        { &FORT_GUID_FILTER_REAUTH_OUT_V4, &FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                L"FortFilterReauthOut4" },
        /* ofilter6 */
        { &FORT_GUID_FILTER_REAUTH_OUT_V6, &FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                L"FortFilterReauthOut6" },
    },
};

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

    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_DISCARD_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_DISCARD_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_IPPACKET_DISCARD_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_IPPACKET_DISCARD_V6);
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

    // TODO: COMPAT: Remove after v4.1.0 (via v4.0.0)
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_STAT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_IN_TRANSPORT_STAT_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_OUT_TRANSPORT_STAT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_OUT_TRANSPORT_STAT_V6);
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
    if (fort_prov_trans_open(&engine) == 0) {

        fort_prov_unregister(engine);

        fort_prov_trans_close(engine, /*status=*/0);
    }
}

static DWORD fort_prov_register_boot_filters(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    if (!boot_conf.boot_filter)
        return STATUS_SUCCESS;

    DWORD status;

    if ((status = fort_prov_add_group(engine, &g_provBootFilters)))
        return status;

    if ((status = fort_prov_add_group(engine, &g_provPersistFilters)))
        return status;

    return STATUS_SUCCESS;
}

static DWORD fort_prov_register_discard_filters(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    if (!boot_conf.stealth_mode)
        return STATUS_SUCCESS;

    return fort_prov_add_group(engine, &g_provDiscardFilters);
}

static DWORD fort_prov_register_callout_filters(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    FORT_PROV_GROUP_ARGS filters = g_provCalloutFilters;

    if (boot_conf.boot_filter) {
        filters.flags = 0;
    }

    return fort_prov_add_group(engine, &filters);
}

static DWORD fort_prov_register_filters(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    DWORD status;

    if ((status = fort_prov_register_boot_filters(engine, boot_conf)))
        return status;

    if ((status = fort_prov_register_discard_filters(engine, boot_conf)))
        return status;

    return fort_prov_register_callout_filters(engine, boot_conf);
}

static DWORD fort_prov_register_provider(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    g_provGlobal.boot_conf = boot_conf;

    FWPM_PROVIDER0 *provider =
            boot_conf.boot_filter ? &g_provGlobal.boot_provider : &g_provGlobal.provider;

    return fort_prov_add_status(FwpmProviderAdd0(engine, provider, NULL), &provider->displayData,
            provider->providerKey);
}

static DWORD fort_prov_register_sublayer(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    FWPM_SUBLAYER0 *sublayer =
            boot_conf.boot_filter ? &g_provGlobal.boot_sublayer : &g_provGlobal.sublayer;

    return fort_prov_add_status(FwpmSubLayerAdd0(engine, sublayer, NULL), &sublayer->displayData,
            sublayer->subLayerKey);
}

FORT_API DWORD fort_prov_register(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    DWORD status;

    if ((status = fort_prov_register_provider(engine, boot_conf)))
        return status;

    if ((status = fort_prov_register_sublayer(engine, boot_conf)))
        return status;

    if ((status = fort_prov_add_group(engine, &g_provAleCallouts)))
        return status;

    if ((status = fort_prov_add_group(engine, &g_provPacketCallouts)))
        return status;

    if ((status = fort_prov_add_group(engine, &g_provDiscardCallouts)))
        return status;

    if ((status = fort_prov_register_filters(engine, boot_conf)))
        return status;

    return 0;
}

FORT_API DWORD fort_prov_reregister(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    DWORD status;

    fort_prov_unregister(engine);

    /* Commit the deletions: some Windows builds (e.g. Insider 29639) fail to add
     * a callout deleted in the same transaction with STATUS_INVALID_PARAMETER */
    if ((status = fort_prov_trans_commit(engine)))
        return status;

    if ((status = fort_prov_trans_begin(engine)))
        return status;

    return fort_prov_register(engine, boot_conf);
}

FORT_API DWORD fort_prov_trans_register(const FORT_PROV_BOOT_CONF boot_conf)
{
    HANDLE engine;
    DWORD status = fort_prov_trans_open(&engine);

    if (status == 0) {

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
    return fort_prov_add_group(engine, &g_provPacketFilters);
}

FORT_API void fort_prov_reauth(HANDLE engine)
{
    if (fort_prov_unregister_reauth_filters(engine) != 0) {
        fort_prov_add_group(engine, &g_provReauthFilters);
    }
}
