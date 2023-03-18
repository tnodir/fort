/* Fort Firewall Driver Provider (Un)Registration */

#include "fortprov.h"

#include "fortioctl.h"

FORT_API DWORD fort_prov_trans_close(HANDLE engine, DWORD status)
{
    if (NT_SUCCESS(status)) {
        status = fort_prov_trans_commit(engine);
    } else {
        fort_prov_trans_abort(engine);
    }

    fort_prov_close(engine);

    return status;
}

static DWORD fort_prov_trans_open_engine(HANDLE transEngine, HANDLE *engine)
{
    DWORD status = 0;

    if (!transEngine) {
        status = fort_prov_open(engine);

        if (status == 0) {
            status = fort_prov_trans_begin(*engine);
        }
    }

    return status;
}

static DWORD fort_prov_trans_close_engine(HANDLE transEngine, HANDLE engine, DWORD status)
{
    if (!transEngine) {
        status = fort_prov_trans_close(engine, status);
    }

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
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_CONNECT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_CONNECT_V6);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_ACCEPT_V4);
    FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_ACCEPT_V6);
}

static DWORD fort_prov_unregister_reauth_filters(HANDLE engine, BOOL force)
{
    const DWORD status = FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_IN_V4);

    if (status == 0 || force) {
        FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_IN_V6);
        FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_OUT_V4);
        FwpmFilterDeleteByKey0(engine, (GUID *) &FORT_GUID_FILTER_REAUTH_OUT_V6);
    }

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
    fort_prov_unregister_reauth_filters(engine, /*force=*/TRUE);

    fort_prov_unregister_callouts(engine);

    FwpmSubLayerDeleteByKey0(engine, (GUID *) &FORT_GUID_SUBLAYER);
    FwpmProviderDeleteByKey0(engine, (GUID *) &FORT_GUID_PROVIDER);
}

static void fort_prov_unregister_flow_filters(HANDLE engine)
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

FORT_API void fort_prov_unregister(HANDLE transEngine)
{
    HANDLE engine = transEngine;

    if (fort_prov_trans_open_engine(transEngine, &engine))
        return;

    fort_prov_unregister_flow_filters(engine);
    fort_prov_unregister_provider(engine);

    fort_prov_trans_close_engine(transEngine, engine, /*status=*/0);
}

FORT_API void fort_prov_flow_unregister(HANDLE transEngine)
{
    HANDLE engine = transEngine;

    if (fort_prov_trans_open_engine(transEngine, &engine))
        return;

    fort_prov_unregister_flow_filters(engine);

    fort_prov_trans_close_engine(transEngine, engine, /*status=*/0);
}

FORT_API FORT_PROV_BOOT_CONF fort_prov_boot_conf(void)
{
    HANDLE engine;
    FORT_PROV_BOOT_CONF boot_conf = { .v = 0 };

    if (!fort_prov_open(&engine)) {
        FWPM_PROVIDER0 *provider;

        if (!FwpmProviderGetByKey0(engine, (GUID *) &FORT_GUID_PROVIDER, &provider)) {
            boot_conf.boot_filter = (provider->flags & FWPM_PROVIDER_FLAG_PERSISTENT);

            if (provider->providerData.size == sizeof(FORT_PROV_BOOT_CONF)) {
                RtlCopyMemory(&boot_conf, provider->providerData.data, sizeof(FORT_PROV_BOOT_CONF));
            }

            FwpmFreeMemory0((void **) &provider);
        }

        fort_prov_close(engine);
    }

    return boot_conf;
}

inline static FWPM_CALLOUT0 fort_prov_init_callout(GUID key, PCWCH name, PCWCH descr, GUID layer)
{
    const FWPM_CALLOUT0 cout = {
        .calloutKey = key,
        .displayData.name = (PWCHAR) name,
        .displayData.description = (PWCHAR) descr,
        .providerKey = (GUID *) &FORT_GUID_PROVIDER,
        .applicableLayer = layer,
    };
    return cout;
}

static DWORD fort_prov_register_callouts(HANDLE engine)
{
    const FWPM_CALLOUT0 callouts[] = {
        /* ocallout4 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_CONNECT_V4, L"FortCalloutConnect4",
                L"Fort Firewall Callout Connect V4", FWPM_LAYER_ALE_AUTH_CONNECT_V4),
        /* ocallout6 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_CONNECT_V6, L"FortCalloutConnect6",
                L"Fort Firewall Callout Connect V6", FWPM_LAYER_ALE_AUTH_CONNECT_V6),
        /* icallout4 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_ACCEPT_V4, L"FortCalloutAccept4",
                L"Fort Firewall Callout Accept V4", FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4),
        /* icallout6 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_ACCEPT_V6, L"FortCalloutAccept6",
                L"Fort Firewall Callout Accept V6", FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6),
        /* scallout4 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_STREAM_V4, L"FortCalloutStream4",
                L"Fort Firewall Callout Stream V4", FWPM_LAYER_STREAM_V4),
        /* scallout6 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_STREAM_V6, L"FortCalloutStream6",
                L"Fort Firewall Callout Stream V6", FWPM_LAYER_STREAM_V6),
        /* dcallout4 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_DATAGRAM_V4, L"FortCalloutDatagram4",
                L"Fort Firewall Callout Datagram V4", FWPM_LAYER_DATAGRAM_DATA_V4),
        /* dcallout6 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_DATAGRAM_V6, L"FortCalloutDatagram6",
                L"Fort Firewall Callout Datagram V6", FWPM_LAYER_DATAGRAM_DATA_V6),
        /* itcallout4 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_IN_TRANSPORT_V4, L"FortCalloutInTransport4",
                L"Fort Firewall Callout Inbound Transport V4", FWPM_LAYER_INBOUND_TRANSPORT_V4),
        /* itcallout6 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_IN_TRANSPORT_V6, L"FortCalloutInTransport6",
                L"Fort Firewall Callout Inbound Transport V6", FWPM_LAYER_INBOUND_TRANSPORT_V6),
        /* otcallout4 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_OUT_TRANSPORT_V4, L"FortCalloutOutTransport4",
                L"Fort Firewall Callout Outbound Transport V4", FWPM_LAYER_OUTBOUND_TRANSPORT_V4),
        /* otcallout6 */
        fort_prov_init_callout(FORT_GUID_CALLOUT_OUT_TRANSPORT_V6, L"FortCalloutOutTransport6",
                L"Fort Firewall Callout Outbound Transport V6", FWPM_LAYER_OUTBOUND_TRANSPORT_V6),
    };

    return fort_prov_add_callouts(
            engine, callouts, /*count=*/sizeof(callouts) / sizeof(callouts[0]));
}

static DWORD fort_prov_register_filters(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    const UINT32 filter_flags =
            boot_conf.boot_filter ? 0 : FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED;

    const FWPM_FILTER0 filters[] = {
        /* ofilter4 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_CONNECT_V4,
                .layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterConnect4",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Connect V4",
                .action.type = FWP_ACTION_CALLOUT_UNKNOWN,
                .action.calloutKey = FORT_GUID_CALLOUT_CONNECT_V4,
        },
        /* ofilter6 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_CONNECT_V6,
                .layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterConnect6",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Connect V6",
                .action.type = FWP_ACTION_CALLOUT_UNKNOWN,
                .action.calloutKey = FORT_GUID_CALLOUT_CONNECT_V6,
        },
        /* ifilter4 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_ACCEPT_V4,
                .layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterAccept4",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Accept V4",
                .action.type = FWP_ACTION_CALLOUT_UNKNOWN,
                .action.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4,
        },
        /* ifilter6 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_ACCEPT_V6,
                .layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterAccept6",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Accept V6",
                .action.type = FWP_ACTION_CALLOUT_UNKNOWN,
                .action.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V6,
        },
    };

    return fort_prov_add_filters(engine, filters, /*count=*/sizeof(filters) / sizeof(filters[0]));
}

static DWORD fort_prov_register_provider(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf)
{
    FWPM_PROVIDER0 provider;
    RtlZeroMemory(&provider, sizeof(FWPM_PROVIDER0));
    provider.flags = boot_conf.boot_filter ? FWPM_PROVIDER_FLAG_PERSISTENT : 0;
    provider.providerKey = FORT_GUID_PROVIDER;
    provider.displayData.name = (PWCHAR) L"FortProvider";
    provider.displayData.description = (PWCHAR) L"Fort Firewall Provider";
    provider.providerData.size = sizeof(FORT_PROV_BOOT_CONF);
    provider.providerData.data = (PUINT8) &boot_conf;
    provider.serviceName = (PWCHAR) L"fortfw";

    FWPM_SUBLAYER0 sublayer;
    RtlZeroMemory(&sublayer, sizeof(FWPM_SUBLAYER0));
    sublayer.subLayerKey = FORT_GUID_SUBLAYER;
    sublayer.displayData.name = (PWCHAR) L"FortSublayer";
    sublayer.displayData.description = (PWCHAR) L"Fort Firewall Sublayer";
    sublayer.providerKey = (GUID *) &FORT_GUID_PROVIDER;

    DWORD status;
    if ((status = FwpmProviderAdd0(engine, &provider, NULL))
            || (status = FwpmSubLayerAdd0(engine, &sublayer, NULL))
            || (status = fort_prov_register_callouts(engine))
            || (status = fort_prov_register_filters(engine, boot_conf))) {
        return status;
    }

    return 0;
}

FORT_API DWORD fort_prov_register(HANDLE transEngine, const FORT_PROV_BOOT_CONF boot_conf)
{
    HANDLE engine = transEngine;
    DWORD status;

    status = fort_prov_trans_open_engine(transEngine, &engine);
    if (status)
        return status;

    status = fort_prov_register_provider(engine, boot_conf);

    status = fort_prov_trans_close_engine(transEngine, engine, status);

    return status;
}

static DWORD fort_prov_flow_register_filters(HANDLE engine)
{
    const UINT32 filter_flags = (FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED
            | FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION);

    const FWPM_FILTER0 filters[] = {
        /* sfilter4 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_STREAM_V4,
                .layerKey = FWPM_LAYER_STREAM_V4,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterStream4",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Stream V4",
                .action.type = FWP_ACTION_CALLOUT_TERMINATING,
                .action.calloutKey = FORT_GUID_CALLOUT_STREAM_V4,
        },
        /* sfilter6 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_STREAM_V6,
                .layerKey = FWPM_LAYER_STREAM_V6,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterStream6",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Stream V6",
                .action.type = FWP_ACTION_CALLOUT_TERMINATING,
                .action.calloutKey = FORT_GUID_CALLOUT_STREAM_V6,
        },
        /* dfilter4 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_DATAGRAM_V4,
                .layerKey = FWPM_LAYER_DATAGRAM_DATA_V4,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterDatagram4",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Datagram V4",
                .action.type = FWP_ACTION_CALLOUT_TERMINATING,
                .action.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V4,
        },
        /* dfilter6 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_DATAGRAM_V6,
                .layerKey = FWPM_LAYER_DATAGRAM_DATA_V6,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterDatagram6",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Datagram V6",
                .action.type = FWP_ACTION_CALLOUT_TERMINATING,
                .action.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V6,
        },
    };

    return fort_prov_add_filters(engine, filters, /*count=*/sizeof(filters) / sizeof(filters[0]));
}

static DWORD fort_prov_flow_register_packet_filters(HANDLE engine)
{
    const UINT32 filter_flags = (FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED
            | FWP_CALLOUT_FLAG_ALLOW_MID_STREAM_INSPECTION);

    const FWPM_FILTER0 filters[] = {
        /* itfilter4 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_IN_TRANSPORT_V4,
                .layerKey = FWPM_LAYER_INBOUND_TRANSPORT_V4,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterInTransport4",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Inbound Transport V4",
                .action.type = FWP_ACTION_CALLOUT_TERMINATING,
                .action.calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V4,
        },
        /* itfilter6 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_IN_TRANSPORT_V6,
                .layerKey = FWPM_LAYER_INBOUND_TRANSPORT_V6,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterInTransport6",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Inbound Transport V6",
                .action.type = FWP_ACTION_CALLOUT_TERMINATING,
                .action.calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V6,
        },
        /* otfilter4 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_OUT_TRANSPORT_V4,
                .layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterOutTransport4",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Outbound Transport V4",
                .action.type = FWP_ACTION_CALLOUT_TERMINATING,
                .action.calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V4,
        },
        /* otfilter6 */
        {
                .flags = filter_flags,
                .filterKey = FORT_GUID_FILTER_OUT_TRANSPORT_V6,
                .layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterOutTransport6",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Outbound Transport V6",
                .action.type = FWP_ACTION_CALLOUT_TERMINATING,
                .action.calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V6,
        },
    };

    return fort_prov_add_filters(engine, filters, /*count=*/sizeof(filters) / sizeof(filters[0]));
}

FORT_API DWORD fort_prov_flow_register(HANDLE transEngine, BOOL filter_packets)
{
    HANDLE engine = transEngine;
    DWORD status;

    status = fort_prov_trans_open_engine(transEngine, &engine);
    if (status)
        return status;

    status = fort_prov_flow_register_filters(engine);

    if (status == 0 && filter_packets) {
        status = fort_prov_flow_register_packet_filters(engine);
    }

    status = fort_prov_trans_close_engine(transEngine, engine, status);

    return status;
}

static DWORD fort_prov_register_reauth_filters(HANDLE engine)
{
    const FWPM_FILTER0 filters[] = {
        /* ifilter4 */
        {
                .filterKey = FORT_GUID_FILTER_REAUTH_IN_V4,
                .layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterReauthIn4",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Reauth Inbound V4",
                .action.type = FWP_ACTION_CONTINUE,
        },
        /* ifilter6 */
        {
                .filterKey = FORT_GUID_FILTER_REAUTH_IN_V6,
                .layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterReauthIn6",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Reauth Inbound V6",
                .action.type = FWP_ACTION_CONTINUE,
        },
        /* ofilter4 */
        {
                .filterKey = FORT_GUID_FILTER_REAUTH_OUT_V4,
                .layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterReauthOut4",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Reauth Outbound V4",
                .action.type = FWP_ACTION_CONTINUE,
        },
        /* ofilter6 */
        {
                .filterKey = FORT_GUID_FILTER_REAUTH_OUT_V6,
                .layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                .subLayerKey = FORT_GUID_SUBLAYER,
                .displayData.name = (PWCHAR) L"FortFilterReauthOut6",
                .displayData.description = (PWCHAR) L"Fort Firewall Filter Reauth Outbound V6",
                .action.type = FWP_ACTION_CONTINUE,
        },
    };

    return fort_prov_add_filters(engine, filters, /*count=*/sizeof(filters) / sizeof(filters[0]));
}

FORT_API DWORD fort_prov_reauth(HANDLE transEngine)
{
    HANDLE engine = transEngine;
    DWORD status;

    status = fort_prov_trans_open_engine(transEngine, &engine);
    if (status)
        return status;

    status = fort_prov_unregister_reauth_filters(engine, /*force=*/FALSE);
    if (status) {
        status = fort_prov_register_reauth_filters(engine);
    }

    status = fort_prov_trans_close_engine(transEngine, engine, status);

    return status;
}
