/* Fort Firewall Callouts */

#include "fortcout.h"

#include "common/fortguid.h"
#include "common/fortprov.h"

#include "fortcout_ale.h"
#include "fortcout_pkt.h"
#include "fortdbg.h"
#include "fortdev.h"
#include "forttrace.h"
#include "fortutl.h"

static struct
{
    BOOL prov_failed; /* the provider filters' changes are failed */
    FORT_CONF_FLAGS prov_old_conf_flags; /* the failed changes' old conf flags */

    FWPS_CALLOUT0 ale_callouts[FORT_STAT_ALE_CALLOUT_IDS_COUNT];
    FWPS_CALLOUT0 packet_callouts[FORT_STAT_PACKET_CALLOUT_IDS_COUNT];
    FWPS_CALLOUT0 discard_callouts[FORT_STAT_DISCARD_CALLOUT_IDS_COUNT];
} g_calloutGlobal;

FORT_API void fort_callout_classify_block(FWPS_CLASSIFY_OUT0 *classifyOut)
{
    classifyOut->actionType = FWP_ACTION_BLOCK;
    classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
}

FORT_API void fort_callout_classify_drop(FWPS_CLASSIFY_OUT0 *classifyOut)
{
    classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

    fort_callout_classify_block(classifyOut);
}

FORT_API void fort_callout_classify_permit(
        const FWPS_FILTER0 *filter, FWPS_CLASSIFY_OUT0 *classifyOut)
{
    classifyOut->actionType = FWP_ACTION_PERMIT;
    if ((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
        classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
    }
}

FORT_API void fort_callout_classify_continue(FWPS_CLASSIFY_OUT0 *classifyOut)
{
    classifyOut->actionType = FWP_ACTION_CONTINUE;
}

static NTSTATUS NTAPI fort_callout_notify(
        FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID *filterKey, FWPS_FILTER0 *filter)
{
    UNUSED(notifyType);
    UNUSED(filterKey);
    UNUSED(filter);

    return STATUS_SUCCESS;
}

static void NTAPI fort_callout_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNUSED(layerId);
    UNUSED(calloutId);
    UNUSED(flowContext);
}

static void fort_callout_init_callout(FWPS_CALLOUT0 *cout, GUID calloutKey,
        FWPS_CALLOUT_CLASSIFY_FN0 classifyFn, FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN0 flowDeleteFn,
        UINT32 flags)
{
    cout->calloutKey = calloutKey;
    cout->flags = flags;
    cout->classifyFn = classifyFn;
    cout->notifyFn = &fort_callout_notify;
    cout->flowDeleteFn = flowDeleteFn;
}

inline static void fort_callout_init_ale_callout(
        FWPS_CALLOUT0 *cout, GUID calloutKey, FWPS_CALLOUT_CLASSIFY_FN0 classifyFn)
{
    fort_callout_init_callout(cout, calloutKey, classifyFn,
            /*flowDeleteFn=*/NULL, /*flags=*/0);
}

static void fort_callout_init_ale_callouts(void)
{
    FWPS_CALLOUT0 *cout = g_calloutGlobal.ale_callouts;

    /* IPv4 connect callout */
    fort_callout_init_ale_callout(cout++, FORT_GUID_CALLOUT_CONNECT_V4, &fort_callout_connect_v4);
    /* IPv6 connect callout */
    fort_callout_init_ale_callout(cout++, FORT_GUID_CALLOUT_CONNECT_V6, &fort_callout_connect_v6);
    /* IPv4 accept callout */
    fort_callout_init_ale_callout(cout++, FORT_GUID_CALLOUT_ACCEPT_V4, &fort_callout_accept_v4);
    /* IPv6 accept callout */
    fort_callout_init_ale_callout(cout++, FORT_GUID_CALLOUT_ACCEPT_V6, &fort_callout_accept_v6);
}

inline static void fort_callout_init_packet_callout(FWPS_CALLOUT0 *cout, GUID calloutKey,
        FWPS_CALLOUT_CLASSIFY_FN0 classifyFn, FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN0 flowDeleteFn)
{
    fort_callout_init_callout(
            cout, calloutKey, classifyFn, flowDeleteFn, FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW);
}

static void fort_callout_init_packet_callouts(void)
{
    FWPS_CALLOUT0 *cout = g_calloutGlobal.packet_callouts;

    /* IPv4 inbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_V4,
            &fort_callout_transport_classify_in, &fort_callout_flow_delete);
    /* IPv6 inbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_V6,
            &fort_callout_transport_classify_in, &fort_callout_flow_delete);
    /* IPv4 outbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_OUT_TRANSPORT_V4,
            &fort_callout_transport_classify_out, &fort_callout_delete);
    /* IPv6 outbound transport callout */
    fort_callout_init_packet_callout(cout++, FORT_GUID_CALLOUT_OUT_TRANSPORT_V6,
            &fort_callout_transport_classify_out, &fort_callout_delete);
}

inline static void fort_callout_init_discard_callout(
        FWPS_CALLOUT0 *cout, GUID calloutKey, FWPS_CALLOUT_CLASSIFY_FN0 classifyFn)
{
    fort_callout_init_callout(cout, calloutKey, classifyFn,
            /*flowDeleteFn=*/NULL, /*flags=*/0);
}

static void fort_callout_init_discard_callouts(void)
{
    FWPS_CALLOUT0 *cout = g_calloutGlobal.discard_callouts;

    /* IPv4 inbound transport discard callout */
    fort_callout_init_discard_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V4,
            &fort_callout_transport_discard_in_v4);
    /* IPv6 inbound transport discard callout */
    fort_callout_init_discard_callout(cout++, FORT_GUID_CALLOUT_IN_TRANSPORT_DISCARD_V6,
            &fort_callout_transport_discard_in_v6);
    /* IPv4 inbound ippacket discard callout */
    fort_callout_init_discard_callout(
            cout++, FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V4, &fort_callout_ippacket_discard_in_v4);
    /* IPv6 inbound ippacket discard callout */
    fort_callout_init_discard_callout(
            cout++, FORT_GUID_CALLOUT_IN_IPPACKET_DISCARD_V6, &fort_callout_ippacket_discard_in_v6);
}

static void fort_callout_init(void)
{
    RtlZeroMemory(&g_calloutGlobal, sizeof(g_calloutGlobal));

    fort_callout_init_ale_callouts();
    fort_callout_init_packet_callouts();
    fort_callout_init_discard_callouts();
}

static NTSTATUS fort_callout_register(
        PDEVICE_OBJECT device, const FWPS_CALLOUT0 *callouts, const PUINT32 calloutIds, int count)
{
    for (int i = 0; i < count; ++i) {
        const NTSTATUS status = FwpsCalloutRegister0(device, &callouts[i], &calloutIds[i]);
        if (!NT_SUCCESS(status)) {
            LOG("Callout Register: Error: %x\n", status);
            TRACE(FORT_CALLOUT_REGISTER_ERROR, status, i, 0);
            return status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS fort_callout_install_ale(PDEVICE_OBJECT device, PFORT_STAT stat)
{
    const PUINT32 calloutIds = &stat->callout_ids[FORT_STAT_ALE_CALLOUT_IDS_INDEX];

    return fort_callout_register(
            device, g_calloutGlobal.ale_callouts, calloutIds, FORT_STAT_ALE_CALLOUT_IDS_COUNT);
}

static NTSTATUS fort_callout_install_packet(PDEVICE_OBJECT device, PFORT_STAT stat)
{
    const PUINT32 calloutIds = &stat->callout_ids[FORT_STAT_PACKET_CALLOUT_IDS_INDEX];

    return fort_callout_register(device, g_calloutGlobal.packet_callouts, calloutIds,
            FORT_STAT_PACKET_CALLOUT_IDS_COUNT);
}

static NTSTATUS fort_callout_install_discard(PDEVICE_OBJECT device, PFORT_STAT stat)
{
    const PUINT32 calloutIds = &stat->callout_ids[FORT_STAT_DISCARD_CALLOUT_IDS_INDEX];

    return fort_callout_register(device, g_calloutGlobal.discard_callouts, calloutIds,
            FORT_STAT_DISCARD_CALLOUT_IDS_COUNT);
}

FORT_API NTSTATUS fort_callout_install(PDEVICE_OBJECT device)
{
    FORT_CHECK_STACK(FORT_CALLOUT_INSTALL);

    PFORT_STAT stat = &fort_device()->stat;

    fort_callout_init();

    NTSTATUS status;

    if (!NT_SUCCESS(status = fort_callout_install_ale(device, stat)))
        return status;

    if (!NT_SUCCESS(status = fort_callout_install_packet(device, stat)))
        return status;

    if (!NT_SUCCESS(status = fort_callout_install_discard(device, stat)))
        return status;

    return STATUS_SUCCESS;
}

FORT_API void fort_callout_remove(void)
{
    FORT_CHECK_STACK(FORT_CALLOUT_REMOVE);

    PFORT_STAT stat = &fort_device()->stat;

    const PUINT32 calloutIds = stat->callout_ids;

    for (int i = 0; i < FORT_STAT_CALLOUT_IDS_COUNT; ++i) {
        PUINT32 calloutId = &calloutIds[i];

        /* Wait for the asynchronously deleting flow contexts */
        while (FwpsCalloutUnregisterById0(*calloutId) == STATUS_DEVICE_BUSY) {
            fort_thread_delay(/*msecs=*/50);
        }

        *calloutId = 0;
    }
}

inline static NTSTATUS fort_callout_force_reauth_prov_flow_filters(HANDLE engine,
        const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags, BOOL force)
{
    const BOOL conf_changed = (old_conf_flags.log_stat != conf_flags.log_stat);

    if (!force && !conf_changed)
        return STATUS_SUCCESS;

    fort_prov_flow_unregister(engine);

    if (!conf_flags.log_stat)
        return STATUS_SUCCESS;

    return fort_prov_flow_register(engine);
}

inline static NTSTATUS fort_callout_force_reauth_prov_recreate(HANDLE engine,
        const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags,
        BOOL *prov_recreated)
{
    const BOOL conf_changed = (old_conf_flags.boot_filter != conf_flags.boot_filter
            || old_conf_flags.stealth_mode != conf_flags.stealth_mode
            || old_conf_flags.filter_locals != conf_flags.filter_locals);

    if (!conf_changed)
        return STATUS_SUCCESS;

    const FORT_PROV_BOOT_CONF boot_conf = {
        .boot_filter = conf_flags.boot_filter,
        .filter_locals = conf_flags.filter_locals,
        .stealth_mode = conf_flags.stealth_mode,
    };

    fort_prov_unregister(engine);

    const NTSTATUS status = fort_prov_register(engine, boot_conf);
    if (status == 0) {
        *prov_recreated = TRUE;
    }

    return status;
}

inline static NTSTATUS fort_callout_force_reauth_prov_filters(
        HANDLE engine, const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags)
{
    NTSTATUS status;

    /* Check provider filters */
    BOOL prov_recreated = FALSE;
    status = fort_callout_force_reauth_prov_recreate(
            engine, old_conf_flags, conf_flags, &prov_recreated);
    if (status != 0)
        return status;

    /* Check flow filter */
    status = fort_callout_force_reauth_prov_flow_filters(engine, old_conf_flags, conf_flags,
            /*force=*/prov_recreated);
    if (status != 0)
        return status;

    /* Force reauth filter */
    fort_prov_reauth(engine);

    return STATUS_SUCCESS;
}

inline static NTSTATUS fort_callout_force_reauth_prov(
        FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags)
{
    NTSTATUS status;

    /* Retry the failed changes: the filters are still by their old conf flags */
    if (g_calloutGlobal.prov_failed) {
        old_conf_flags = g_calloutGlobal.prov_old_conf_flags;
    }

    HANDLE engine;
    status = fort_prov_trans_open(&engine);
    if (NT_SUCCESS(status)) {
        status = fort_callout_force_reauth_prov_filters(engine, old_conf_flags, conf_flags);

        status = fort_prov_trans_close(engine, status);
    }

    g_calloutGlobal.prov_failed = !NT_SUCCESS(status);
    g_calloutGlobal.prov_old_conf_flags = old_conf_flags;

    return status;
}

FORT_API NTSTATUS fort_callout_force_reauth(const FORT_CONF_FLAGS old_conf_flags)
{
    FORT_CHECK_STACK(FORT_CALLOUT_FORCE_REAUTH);

    NTSTATUS status;

    const FORT_CONF_FLAGS conf_flags = fort_device_conf_flags(&fort_device()->conf);

    /* Handle log_stat */
    fort_stat_log_update(&fort_device()->stat, conf_flags.log_stat);

    /* Run the log_timer */
    fort_timer_set_running(&fort_device()->log_timer, /*run=*/conf_flags.log_stat);

    /* Reauth provider filters */
    status = fort_callout_force_reauth_prov(old_conf_flags, conf_flags);

    if (!NT_SUCCESS(status)) {
        LOG("Callout Reauth: Error: %x\n", status);
        TRACE(FORT_CALLOUT_CALLOUT_REAUTH_ERROR, status, 0, 0);
    }

    return status;
}

inline static void fort_callout_update_system_time(
        PFORT_STAT stat, PFORT_BUFFER buf, PFORT_IRP_INFO irp_info)
{
    LARGE_INTEGER system_time;
    KeQuerySystemTime(&system_time);

    if (stat->system_time.QuadPart == system_time.QuadPart)
        return;

    stat->system_time = system_time;

    const INT64 unix_time = fort_system_to_unix_time(system_time.QuadPart);

    const UCHAR old_stat_flags = fort_stat_flags_set(stat, FORT_STAT_SYSTEM_TIME_CHANGED, FALSE);
    const BOOL system_time_changed = (old_stat_flags & FORT_STAT_SYSTEM_TIME_CHANGED) != 0;

    fort_buffer_system_time_write_locked(buf, irp_info, unix_time, system_time_changed);
}

inline static void fort_callout_flush_stat_traf(
        PFORT_STAT stat, PFORT_BUFFER buf, PFORT_IRP_INFO irp_info)
{
    while (stat->proc_active_count != 0) {
        const UINT16 proc_count = (stat->proc_active_count < FORT_LOG_STAT_BUFFER_PROC_COUNT)
                ? stat->proc_active_count
                : FORT_LOG_STAT_BUFFER_PROC_COUNT;
        const UINT32 len = FORT_LOG_STAT_SIZE(proc_count);
        PCHAR out;

        const NTSTATUS status = fort_buffer_prepare(buf, len, &out, irp_info);
        if (!NT_SUCCESS(status)) {
            LOG("Callout Timer: Error: %x\n", status);
            TRACE(FORT_CALLOUT_CALLOUT_TIMER_ERROR, status, 0, 0);
            break;
        }

        fort_log_stat_traf_header_write(out, proc_count);
        out += FORT_LOG_STAT_HEADER_SIZE;

        fort_stat_traf_flush(stat, proc_count, out);
    }
}

FORT_API void fort_callout_timer(void)
{
    FORT_CHECK_STACK(FORT_CALLOUT_TIMER);

    PFORT_BUFFER buf = &fort_device()->buffer;
    PFORT_STAT stat = &fort_device()->stat;

    FORT_IRP_INFO irp_info = { .irp = NULL };

    /* Called also by the packet's classify at PASSIVE level, so the locks raise the IRQL */

    /* Lock buffer */
    KLOCK_QUEUE_HANDLE buf_lock_queue;
    KeAcquireInStackQueuedSpinLock(&buf->lock, &buf_lock_queue);

    /* Lock stat */
    KLOCK_QUEUE_HANDLE stat_lock_queue;
    KeAcquireInStackQueuedSpinLock(&stat->lock, &stat_lock_queue);

    /* Get current Unix time */
    fort_callout_update_system_time(stat, buf, &irp_info);

    /* Flush traffic statistics */
    fort_callout_flush_stat_traf(stat, buf, &irp_info);

    /* Unlock stat */
    KeReleaseInStackQueuedSpinLock(&stat_lock_queue);

    /* Flush pending buffer */
    if (irp_info.irp == NULL) {
        fort_buffer_flush_pending(buf, &irp_info);
    }

    /* Unlock buffer */
    KeReleaseInStackQueuedSpinLock(&buf_lock_queue);

    if (irp_info.irp != NULL) {
        fort_buffer_irp_clear_pending(&irp_info);
        fort_request_complete_info(&irp_info, STATUS_SUCCESS);
    }
}
