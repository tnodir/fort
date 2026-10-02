/* Fort Firewall Pending Packets */

#include "fortpkt_pending.h"

#include "fortdev.h"
#include "fortthr.h"
#include "fortutl.h"

inline static PFORT_PENDING_PACKET fort_pending_packet_new(void)
{
    return fort_mem_alloc(sizeof(FORT_PENDING_PACKET), FORT_PACKET_POOL_TAG);
}

inline static void fort_pending_packet_del(PFORT_PENDING_PACKET pkt)
{
    fort_mem_free(pkt, FORT_PACKET_POOL_TAG);
}

FORT_API void fort_pending_packet_free(PFORT_PENDING_PACKET pkt)
{
    fort_packet_free(&pkt->io);

    fort_pending_packet_del(pkt);
}

static PFORT_PENDING_PROC fort_pending_proc_find_locked(PFORT_PENDING pending, UINT32 process_id)
{
    PFORT_PENDING_PROC proc = pending->procs_head;

    for (; proc != NULL; proc = proc->next) {
        if (proc->process_id == process_id)
            return proc;
    }

    return NULL;
}

static BOOL fort_pending_proc_check_limits(PFORT_PENDING pending, UINT32 process_id)
{
    UINT16 proc_count = 0;
    UINT16 packet_count = 0;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    proc_count = pending->proc_count;

    PFORT_PENDING_PROC proc = fort_pending_proc_find_locked(pending, process_id);
    if (proc != NULL) {
        packet_count = proc->packet_count;
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return proc_count < FORT_PENDING_PROC_COUNT_MAX
            && packet_count < FORT_PENDING_PROC_PACKET_COUNT_MAX;
}

static PFORT_PENDING_PROC fort_pending_proc_get_locked(PFORT_PENDING pending, UINT32 process_id)
{
    PFORT_PENDING_PROC proc;

    if (pending->proc_free != NULL) {
        proc = pending->proc_free;
        pending->proc_free = proc->next;
    } else {
        const tommy_size_t size = tommy_arrayof_size(&pending->procs);

        /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
        if (tommy_arrayof_grow(&pending->procs, size + 1), 0)
            return NULL;

        proc = tommy_arrayof_ref(&pending->procs, size);
    }

    proc->packets_head = NULL;
    proc->packet_count = 0;
    proc->process_id = process_id;

#if 0 // TODO
    proc->next = pending->procs_head;
    pending->procs_head = proc;
#endif

    pending->proc_count++;

    return proc;
}

static PFORT_PENDING_PROC fort_pending_proc_get_check(PFORT_PENDING pending, UINT32 process_id)
{
    PFORT_PENDING_PROC proc = fort_pending_proc_find_locked(pending, process_id);

    if (proc == NULL) {
        return fort_pending_proc_get_locked(pending, process_id);
    }

    if (proc->packet_count >= FORT_PENDING_PROC_PACKET_COUNT_MAX)
        return NULL;

    return proc;
}

static void fort_pending_proc_put_locked(PFORT_PENDING pending, PFORT_PENDING_PROC proc)
{
    pending->proc_count--;

    proc->next = pending->proc_free;
    pending->proc_free = proc;
}

static NTSTATUS fort_pending_proc_add_packet_locked(PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca,
        PCFORT_CONF_META_CONN conn, PFORT_PENDING_PACKET pkt)
{
    /* Create the Pending Process */
    PFORT_PENDING_PROC proc = fort_pending_proc_get_check(pending, conn->process_id);
    if (proc == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    const NTSTATUS status =
            FwpsPendOperation0(ca->inMetaValues->completionHandle, &pkt->completion_context);

    if (!NT_SUCCESS(status)) {
        if (proc->packet_count == 0) {
            fort_pending_proc_put_locked(pending, proc);
        }
        return status;
    }

    proc->packet_count++;

    pkt->next = proc->packets_head;
    proc->packets_head = pkt;

    return STATUS_SUCCESS;
}

static NTSTATUS fort_pending_proc_add_packet(PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca,
        PCFORT_CONF_META_CONN conn, PFORT_PENDING_PACKET pkt)
{
    NTSTATUS status;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    status = fort_pending_proc_add_packet_locked(pending, ca, conn, pkt);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return status;
}

static void fort_pending_init(PFORT_PENDING pending)
{
    tommy_arrayof_init(&pending->procs, sizeof(FORT_PENDING_PROC));
}

FORT_API void fort_pending_open(PFORT_PENDING pending)
{
    FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport4_in_id);
    FwpsInjectionHandleCreate0(
            AF_INET, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport4_out_id);
    FwpsInjectionHandleCreate0(
            AF_INET6, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport6_in_id);
    FwpsInjectionHandleCreate0(
            AF_INET6, FWPS_INJECTION_TYPE_TRANSPORT, &pending->injection_transport6_out_id);

    fort_pending_init(pending);

    KeInitializeSpinLock(&pending->lock);
}

static void fort_pending_done(PFORT_PENDING pending)
{
    tommy_arrayof_done(&pending->procs);
}

static void fort_pending_wait_injections(PFORT_PENDING pending)
{
    for (;;) {
        const LONG inject_count = InterlockedOr(&pending->inject_count, 0);

        fort_thread_delay(/*msecs=*/10);

        if (inject_count == 0)
            break; /* Check the extra one time to ensure the completion's exit */
    }
}

FORT_API void fort_pending_close(PFORT_PENDING pending)
{
    /* Wait for the asynchronous injections before their handles' destruction */
    fort_pending_wait_injections(pending);

    fort_pending_done(pending);

    FwpsInjectionHandleDestroy0(pending->injection_transport4_in_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport4_out_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport6_in_id);
    FwpsInjectionHandleDestroy0(pending->injection_transport6_out_id);
}

static void fort_pending_clear_locked(PFORT_PENDING pending)
{
    if (pending->proc_count == 0)
        return;

    pending->proc_count = 0;
    pending->proc_free = NULL;
    pending->procs_head = NULL;

    fort_pending_done(pending);
    fort_pending_init(pending);
}

FORT_API void fort_pending_clear(PFORT_PENDING pending)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&pending->lock, &lock_queue);

    fort_pending_clear_locked(pending);

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API BOOL fort_pending_add_packet(
        PFORT_PENDING pending, PCFORT_CALLOUT_ARG ca, PCFORT_CONF_META_CONN conn)
{
    NTSTATUS status;

    /* Skip self injected packet */
    if (fort_packet_injected_by_self(ca))
        return FALSE;

    /* Check the Process's Limits */
    if (!fort_pending_proc_check_limits(pending, conn->process_id))
        return FALSE;

    /* Create the Packet */
    PFORT_PENDING_PACKET pkt = fort_pending_packet_new();
    if (pkt == NULL)
        return FALSE;

    RtlZeroMemory(pkt, sizeof(FORT_PENDING_PACKET));

    const UCHAR ipsec_flag = fort_packet_is_ipsec_protected(ca) ? FORT_PACKET_IPSEC_PROTECTED : 0;

    status = fort_packet_fill(ca, &pkt->io, ipsec_flag | FORT_PACKET_TYPE_PENDING);
    if (NT_SUCCESS(status)) {
        /* Add the Packet to Pending Process */
        status = fort_pending_proc_add_packet(pending, ca, conn, pkt);
    }

    if (!NT_SUCCESS(status)) {
        fort_pending_packet_free(pkt);
        return FALSE;
    }

    return TRUE;
}
