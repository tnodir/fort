/* Fort Firewall Driver */

#define NDIS_WDM	1
#define NDIS630		1

#define WIN9X_COMPAT_SPINLOCK  /* XXX: Support Windows 7: KeInitializeSpinLock() */

#include <wdm.h>
#include <fwpmk.h>
#include <fwpsk.h>
#include <stddef.h>
#include <ntrxdef.h>

#include "../common/common.h"
#include "fortdrv.h"

#define FORT_DEVICE_POOL_TAG	'DwfF'

#include "../common/fortconf.c"
#include "../common/fortlog.c"
#include "../common/fortprov.c"
#include "forttds.c"
#include "fortbuf.c"
#include "fortpkt.c"
#include "fortstat.c"
#include "forttmr.c"

typedef struct fort_conf_ref {
  UINT32 volatile refcount;

  FORT_CONF conf;
} FORT_CONF_REF, *PFORT_CONF_REF;

typedef struct fort_device {
  UINT32 prov_boot	: 1;
  UINT32 is_opened	: 1;
  UINT32 was_conf	: 1;
  UINT32 power_off	: 1;

  UINT32 connect4_id;
  UINT32 accept4_id;

  FORT_CONF_FLAGS volatile conf_flags;
  PFORT_CONF_REF volatile conf_ref;
  KSPIN_LOCK conf_lock;

  PCALLBACK_OBJECT power_cb_obj;
  PVOID power_cb_reg;

  FORT_BUFFER buffer;
  FORT_STAT stat;
  FORT_DEFER defer;
  FORT_TIMER timer;
} FORT_DEVICE, *PFORT_DEVICE;

static PFORT_DEVICE g_device = NULL;


static PFORT_CONF_REF
fort_conf_ref_new (const PFORT_CONF conf, ULONG len)
{
  const ULONG ref_len = len + offsetof(FORT_CONF_REF, conf);
  PFORT_CONF_REF conf_ref = fort_mem_alloc(ref_len, FORT_DEVICE_POOL_TAG);

  if (conf_ref != NULL) {
    conf_ref->refcount = 0;

    RtlCopyMemory(&conf_ref->conf, conf, len);
  }

  return conf_ref;
}

static void
fort_conf_ref_put (PFORT_CONF_REF conf_ref)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&g_device->conf_lock, &lock_queue);
  {
    const UINT32 refcount = --conf_ref->refcount;

    if (refcount == 0 && conf_ref != g_device->conf_ref) {
      fort_mem_free(conf_ref, FORT_DEVICE_POOL_TAG);
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static PFORT_CONF_REF
fort_conf_ref_take (void)
{
  PFORT_CONF_REF conf_ref;
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&g_device->conf_lock, &lock_queue);
  {
    conf_ref = g_device->conf_ref;
    if (conf_ref) {
      ++conf_ref->refcount;
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return conf_ref;
}

static FORT_CONF_FLAGS
fort_conf_ref_set (PFORT_CONF_REF conf_ref)
{
  PFORT_CONF_REF old_conf_ref;
  FORT_CONF_FLAGS old_conf_flags;
  KLOCK_QUEUE_HANDLE lock_queue;

  old_conf_ref = fort_conf_ref_take();

  KeAcquireInStackQueuedSpinLock(&g_device->conf_lock, &lock_queue);
  {
    g_device->conf_ref = conf_ref;

    if (old_conf_ref == NULL) {
      RtlZeroMemory(&old_conf_flags, sizeof(FORT_CONF_FLAGS));
      old_conf_flags.prov_boot = g_device->prov_boot;
    }

    if (conf_ref != NULL) {
      const PFORT_CONF_FLAGS conf_flags = &conf_ref->conf.flags;

      g_device->prov_boot = conf_flags->prov_boot;
      g_device->was_conf = TRUE;

      g_device->conf_flags = *conf_flags;
    } else {
      RtlZeroMemory((void *) &g_device->conf_flags, sizeof(FORT_CONF_FLAGS));
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  if (old_conf_ref != NULL) {
    old_conf_flags = old_conf_ref->conf.flags;
    fort_conf_ref_put(old_conf_ref);
  }

  return old_conf_flags;
}

static FORT_CONF_FLAGS
fort_conf_ref_flags_set (const PFORT_CONF_FLAGS conf_flags)
{
  FORT_CONF_FLAGS old_conf_flags;
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&g_device->conf_lock, &lock_queue);
  {
    PFORT_CONF_REF conf_ref = g_device->conf_ref;

    if (conf_ref != NULL) {
      PFORT_CONF conf = &conf_ref->conf;

      old_conf_flags = conf->flags;
      conf->flags = *conf_flags;

      fort_conf_app_perms_mask_init(conf);

      g_device->prov_boot = conf_flags->prov_boot;

      g_device->conf_flags = *conf_flags;
    } else {
      RtlZeroMemory(&old_conf_flags, sizeof(FORT_CONF_FLAGS));
      old_conf_flags.prov_boot = g_device->prov_boot;

      RtlZeroMemory((void *) &g_device->conf_flags, sizeof(FORT_CONF_FLAGS));
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return old_conf_flags;
}

static void
fort_callout_classify_block (FWPS_CLASSIFY_OUT0 *classifyOut)
{
  classifyOut->actionType = FWP_ACTION_BLOCK;
  classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
}

static void
fort_callout_classify_drop (FWPS_CLASSIFY_OUT0 *classifyOut)
{
  classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

  fort_callout_classify_block(classifyOut);
}

static void
fort_callout_classify_permit (const FWPS_FILTER0 *filter,
                              FWPS_CLASSIFY_OUT0 *classifyOut)
{
  classifyOut->actionType = FWP_ACTION_PERMIT;
  if ((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
    classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
  }
}

static void
fort_callout_classify_continue (FWPS_CLASSIFY_OUT0 *classifyOut)
{
  classifyOut->actionType = FWP_ACTION_CONTINUE;
}

static void
fort_callout_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                          const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                          const FWPS_FILTER0 *filter,
                          FWPS_CLASSIFY_OUT0 *classifyOut,
                          int flagsField, int remoteIpField)
{
  PFORT_CONF_REF conf_ref;
  PVOID path;
  FORT_CONF_FLAGS conf_flags;
  UINT32 flags;
  UINT32 remote_ip;
  UINT32 process_id;
  UINT32 path_len;

  PIRP irp = NULL;
  ULONG_PTR info;

  flags = inFixedValues->incomingValue[flagsField].value.uint32;
  remote_ip = inFixedValues->incomingValue[remoteIpField].value.uint32;

  if ((flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
      || remote_ip == 0xFFFFFFFF) {  /* Local broadcast */
    fort_callout_classify_permit(filter, classifyOut);
    return;
  }

  conf_ref = fort_conf_ref_take();

  if (conf_ref == NULL) {
    if (g_device->prov_boot || ((flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE)
        && !g_device->was_conf)) {  /* Block existing flows after driver installation to use flow-contexts */
      fort_callout_classify_block(classifyOut);
    } else {
      fort_callout_classify_continue(classifyOut);
    }
    return;
  }

  conf_flags = conf_ref->conf.flags;

  if (conf_flags.stop_traffic)
    goto block;

  if (!conf_flags.filter_enabled
      || !fort_conf_ip_is_inet(&conf_ref->conf, remote_ip))
    goto permit;

  if (conf_flags.stop_inet_traffic)
    goto block;

  process_id = (UINT32) inMetaValues->processId;
  path_len = inMetaValues->processPath->size - sizeof(WCHAR);  /* chop terminating zero */
  path = inMetaValues->processPath->data;

  if (fort_conf_ip_inet_included(&conf_ref->conf, remote_ip)) {
    const int app_index = fort_conf_app_index(&conf_ref->conf, path_len, path);

    if (!fort_conf_app_blocked(&conf_ref->conf, app_index)) {
      if (conf_flags.log_stat) {
        const UINT64 flowId = inMetaValues->flowHandle;
        const UCHAR group_index = fort_conf_app_group_index(
          &conf_ref->conf, app_index);
        BOOL is_new_proc = FALSE;
        NTSTATUS status;

        status = fort_stat_flow_associate(&g_device->stat,
          flowId, process_id, group_index, &is_new_proc);

        if (!NT_SUCCESS(status)) {
          DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                     "FORT: Classify v4: Flow assoc. error: %x\n", status);
        } else if (is_new_proc) {
          fort_buffer_proc_new_write(&g_device->buffer,
            process_id, path_len, path, &irp, &info);
        }
      }
      goto permit;
    }
  }

  if (conf_flags.log_blocked) {
    fort_buffer_blocked_write(&g_device->buffer,
      remote_ip, process_id, path_len, path, &irp, &info);
  }

 block:
  fort_callout_classify_block(classifyOut);
  goto end;

 permit:
  fort_callout_classify_permit(filter, classifyOut);

 end:
  fort_conf_ref_put(conf_ref);

  if (irp != NULL) {
    fort_request_complete_info(irp, STATUS_SUCCESS, info);
  }
}

static void
fort_callout_connect_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                         const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                         void *layerData,
                         const FWPS_FILTER0 *filter,
                         UINT64 flowContext,
                         FWPS_CLASSIFY_OUT0 *classifyOut)
{
  UNUSED(layerData);
  UNUSED(flowContext);

  fort_callout_classify_v4(inFixedValues, inMetaValues, filter, classifyOut,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS);
}

static void
fort_callout_accept_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                        void *layerData,
                        const FWPS_FILTER0 *filter,
                        UINT64 flowContext,
                        FWPS_CLASSIFY_OUT0 *classifyOut)
{
  UNUSED(layerData);
  UNUSED(flowContext);

  fort_callout_classify_v4(inFixedValues, inMetaValues, filter, classifyOut,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS);
}

static NTSTATUS NTAPI
fort_callout_notify (FWPS_CALLOUT_NOTIFY_TYPE notifyType,
                     const GUID *filterKey, const FWPS_FILTER0 *filter)
{
  UNUSED(notifyType);
  UNUSED(filterKey);
  UNUSED(filter);

  return STATUS_SUCCESS;
}

static void
fort_callout_flow_classify_v4 (const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                               const FWPS_FILTER0 *filter,
                               UINT64 flowContext,
                               FWPS_CLASSIFY_OUT0 *classifyOut,
                               UINT32 dataSize, BOOL inbound)
{
  const UINT32 headerSize = inbound ? inMetaValues->transportHeaderSize : 0;

  if (fort_stat_flow_classify(&g_device->stat, flowContext,
      headerSize + dataSize, inbound)) {
    fort_callout_classify_drop(classifyOut);
  } else {
    fort_callout_classify_permit(filter, classifyOut);
  }
}

static void
fort_callout_stream_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                                 const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                                 const FWPS_STREAM_CALLOUT_IO_PACKET0 *packet,
                                 const FWPS_FILTER0 *filter,
                                 UINT64 flowContext,
                                 FWPS_CLASSIFY_OUT0 *classifyOut)
{
  const FWPS_STREAM_DATA0 *streamData = packet->streamData;
  const UINT32 dataSize = (UINT32) streamData->dataLength;

  const BOOL inbound = (streamData->flags & FWPS_STREAM_FLAG_RECEIVE) != 0;

  UNUSED(inFixedValues);

  fort_callout_flow_classify_v4(inMetaValues, filter, flowContext,
    classifyOut, dataSize, inbound);
}

static void
fort_callout_datagram_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                                   const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                                   const PNET_BUFFER_LIST netBufList,
                                   const FWPS_FILTER0 *filter,
                                   UINT64 flowContext,
                                   FWPS_CLASSIFY_OUT0 *classifyOut)
{
  const PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList);
  const UINT32 dataSize = NET_BUFFER_DATA_LENGTH(netBuf);

  const FWP_DIRECTION direction = (FWP_DIRECTION) inFixedValues->incomingValue[
    FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION].value.uint8;
  const BOOL inbound = (direction == FWP_DIRECTION_INBOUND);

  UNUSED(inFixedValues);

  fort_callout_flow_classify_v4(inMetaValues, filter, flowContext,
    classifyOut, dataSize, inbound);
}

static void
fort_callout_flow_delete_v4 (UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
  UNUSED(layerId);
  UNUSED(calloutId);

  fort_stat_flow_delete(&g_device->stat, flowContext);
}

static void
fort_transport_inject_complete (PFORT_PACKET pkt,
                                PNET_BUFFER_LIST clonedNetBufList,
                                BOOLEAN dispatchLevel)
{
  fort_defer_free(&g_device->defer, pkt, clonedNetBufList, dispatchLevel);
}

static void
fort_callout_transport_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                                    const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                                    PNET_BUFFER_LIST netBufList,
                                    const FWPS_FILTER0 *filter,
                                    UINT64 flowContext,
                                    FWPS_CLASSIFY_OUT0 *classifyOut,
                                    int ipProtoField, BOOL inbound)
{
  PNET_BUFFER netBuf;
  const IPPROTO ip_proto = (IPPROTO) inFixedValues->incomingValue[
    ipProtoField].value.uint8;
  const BOOL is_udp = (ip_proto == IPPROTO_UDP);

  if (!(is_udp || FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues,
        FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED))
      && netBufList != NULL
      && (netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList)) != NULL) {
    PFORT_STAT_FLOW flow = (PFORT_STAT_FLOW) flowContext;

    const UCHAR defer_flag = inbound
      ? FORT_STAT_FLOW_DEFER_IN : FORT_STAT_FLOW_DEFER_OUT;
    const UCHAR flow_flags = FORT_STAT_FLOW_SPEED_LIMIT | defer_flag;
    const BOOL defer_flow = (fort_stat_flow_flags(flow) & flow_flags) == flow_flags
      && !g_device->power_off;

    /* Position in the packet data:
     * FWPS_LAYER_INBOUND_TRANSPORT_V4: The beginning of the data.
     * FWPS_LAYER_OUTBOUND_TRANSPORT_V4: The beginning of the transport header.
     */
    const UINT32 headerOffset = inbound ? 0 : sizeof(TCP_HEADER);

    /* Ignore TCP RST-packets */
    #if 0
    const BOOL ignore_tcp_rst = inbound && g_device->conf_flags.ignore_tcp_rst;

    if (ignore_tcp_rst) {
      TCP_HEADER buf;
      PTCP_HEADER tcpHeader;
      UINT32 tcpFlags;

      if (headerOffset != 0) {
        NdisRetreatNetBufferDataStart(netBuf, headerOffset, 0, NULL);
      }

      tcpHeader = NdisGetDataBuffer(netBuf, sizeof(TCP_HEADER), &buf, 1, 0);
      tcpFlags = tcpHeader ? tcpHeader->flags : 0;

      if (headerOffset != 0) {
        NdisAdvanceNetBufferDataStart(netBuf, headerOffset, FALSE, NULL);
      }

      if (tcpHeader == NULL)
        goto permit;

      if (tcpFlags & TCP_FLAG_RST)
        goto block;
    }
    #endif

    /* Defer TCP zero ACK-packets */
    if (defer_flow && NET_BUFFER_DATA_LENGTH(netBuf) == headerOffset) {
      const NTSTATUS status = fort_defer_add(&g_device->defer,
        inFixedValues, inMetaValues, netBufList, inbound);

      if (NT_SUCCESS(status))
        goto block;

      if (status == STATUS_CANT_TERMINATE_SELF) {
        /* Clear ACK deferring */
        fort_stat_flow_flags_clear(flow, defer_flag);
      }
    }
  }

 /* permit: */
  fort_callout_classify_permit(filter, classifyOut);
  return;

 block:
  fort_callout_classify_drop(classifyOut);
  return;
}

static void
fort_callout_in_transport_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                                       const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                                       const PNET_BUFFER_LIST netBufList,
                                       const FWPS_FILTER0 *filter,
                                       UINT64 flowContext,
                                       FWPS_CLASSIFY_OUT0 *classifyOut)
{
  fort_callout_transport_classify_v4(inFixedValues, inMetaValues, netBufList,
    filter, flowContext, classifyOut,
    FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_PROTOCOL,
    TRUE);
}

static void
fort_callout_out_transport_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                                        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                                        const PNET_BUFFER_LIST netBufList,
                                        const FWPS_FILTER0 *filter,
                                        UINT64 flowContext,
                                        FWPS_CLASSIFY_OUT0 *classifyOut)
{
  fort_callout_transport_classify_v4(inFixedValues, inMetaValues, netBufList,
    filter, flowContext, classifyOut,
    FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_PROTOCOL,
    FALSE);
}

static void
fort_callout_delete_v4 (UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
  UNUSED(layerId);
  UNUSED(calloutId);
  UNUSED(flowContext);
}

static NTSTATUS
fort_callout_install (PDEVICE_OBJECT device)
{
  FWPS_CALLOUT0 c;
  NTSTATUS status;

  RtlZeroMemory(&c, sizeof(FWPS_CALLOUT0));

  c.notifyFn = fort_callout_notify;

  /* IPv4 connect callout */
  c.calloutKey = FORT_GUID_CALLOUT_CONNECT_V4;
  c.classifyFn = fort_callout_connect_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->connect4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Register Connect V4: Error: %x\n", status);
    return status;
  }

  /* IPv4 accept callout */
  c.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4;
  c.classifyFn = fort_callout_accept_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->accept4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Register Accept V4: Error: %x\n", status);
    return status;
  }

  /* IPv4 stream callout */
  c.calloutKey = FORT_GUID_CALLOUT_STREAM_V4;
  c.classifyFn = fort_callout_stream_classify_v4;

  c.flowDeleteFn = fort_callout_flow_delete_v4;
  c.flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW;

  status = FwpsCalloutRegister0(device, &c, &g_device->stat.stream4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Register Stream V4: Error: %x\n", status);
    return status;
  }

  /* IPv4 datagram callout */
  c.calloutKey = FORT_GUID_CALLOUT_DATAGRAM_V4;
  c.classifyFn = fort_callout_datagram_classify_v4;

  c.flowDeleteFn = fort_callout_delete_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->stat.datagram4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Register Datagram V4: Error: %x\n", status);
    return status;
  }

  /* IPv4 inbound transport callout */
  c.calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V4;
  c.classifyFn = fort_callout_in_transport_classify_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->stat.in_transport4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Register Inbound Transport V4: Error: %x\n", status);
    return status;
  }

  /* IPv4 outbound transport callout */
  c.calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V4;
  c.classifyFn = fort_callout_out_transport_classify_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->stat.out_transport4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Register Outbound Transport V4: Error: %x\n", status);
    return status;
  }

  return STATUS_SUCCESS;
}

static void
fort_callout_remove (void)
{
  if (g_device->connect4_id) {
    FwpsCalloutUnregisterById0(g_device->connect4_id);
    g_device->connect4_id = 0;
  }

  if (g_device->accept4_id) {
    FwpsCalloutUnregisterById0(g_device->accept4_id);
    g_device->accept4_id = 0;
  }

  if (g_device->stat.stream4_id) {
    FwpsCalloutUnregisterById0(g_device->stat.stream4_id);
    g_device->stat.stream4_id = 0;
  }

  if (g_device->stat.datagram4_id) {
    FwpsCalloutUnregisterById0(g_device->stat.datagram4_id);
    g_device->stat.datagram4_id = 0;
  }

  if (g_device->stat.in_transport4_id) {
    FwpsCalloutUnregisterById0(g_device->stat.in_transport4_id);
    g_device->stat.in_transport4_id = 0;
  }

  if (g_device->stat.out_transport4_id) {
    FwpsCalloutUnregisterById0(g_device->stat.out_transport4_id);
    g_device->stat.out_transport4_id = 0;
  }
}

static void
fort_callout_defer_flush (BOOL dispatchLevel)
{
  fort_defer_flush(&g_device->defer, fort_transport_inject_complete, dispatchLevel);
}

static NTSTATUS
fort_callout_force_reauth (PDEVICE_OBJECT device,
                           const FORT_CONF_FLAGS old_conf_flags,
                           const FORT_CONF_FLAGS conf_flags)
{
  PFORT_STAT stat = &g_device->stat;

  HANDLE engine;
  NTSTATUS status;

  UNUSED(device);

  fort_timer_update(&g_device->timer, FALSE);

  if (old_conf_flags.log_stat != conf_flags.log_stat) {
    fort_stat_update(stat, conf_flags.log_stat);

    if (!conf_flags.log_stat) {
      fort_callout_defer_flush(FALSE);
    }
  }

  if ((status = fort_prov_open(&engine)))
    goto end;

  fort_prov_trans_begin(engine);

  /* Check provider filters */
  if (old_conf_flags.prov_boot != conf_flags.prov_boot) {
    fort_prov_unregister(engine);

    if ((status = fort_prov_register(engine, conf_flags.prov_boot)))
      goto cleanup;

    goto stat_prov;
  }

  /* Check flow filter */
  if (old_conf_flags.log_stat != conf_flags.log_stat) {
    if (old_conf_flags.log_stat) {
      fort_prov_flow_unregister(engine);
    }

 stat_prov:
    if (conf_flags.log_stat) {
      if ((status = fort_prov_flow_register(engine, stat->limit_bits)))
        goto cleanup;
    }
  }

  /* Force reauth filter */
  if ((status = fort_prov_reauth(engine)))
    goto cleanup;

  fort_timer_update(&g_device->timer,
    (conf_flags.log_blocked || conf_flags.log_stat));

 cleanup:
  if (NT_SUCCESS(status)) {
    status = fort_prov_trans_commit(engine);
  } else {
    fort_prov_trans_abort(engine);
  }

  fort_prov_close(engine);

 end:
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Callout Reauth: Error: %x\n", status);
  }

  return status;
}

static void
fort_callout_timer (void)
{
  PFORT_BUFFER buf = &g_device->buffer;
  PFORT_STAT stat = &g_device->stat;

  KLOCK_QUEUE_HANDLE buf_lock_queue;
  KLOCK_QUEUE_HANDLE stat_lock_queue;

  PIRP irp = NULL;
  ULONG_PTR info;

  /* Lock buffer */
  fort_buffer_dpc_begin(buf, &buf_lock_queue);

  /* Lock stat */
  fort_stat_dpc_begin(stat, &stat_lock_queue);

  /* Flush traffic statistics */
  while (stat->proc_active_count) {
    const UINT16 proc_count =
      (stat->proc_active_count < FORT_LOG_STAT_BUFFER_PROC_COUNT)
      ? stat->proc_active_count : FORT_LOG_STAT_BUFFER_PROC_COUNT;
    const UINT32 len = FORT_LOG_STAT_SIZE(proc_count);
    PCHAR out;
    NTSTATUS status;

    status = fort_buffer_prepare(buf, len, &out, &irp, &info);
    if (!NT_SUCCESS(status)) {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                 "FORT: Callout Timer: Error: %x\n", status);
      break;
    }

    fort_log_stat_traf_header_write(out, proc_count);
    out += FORT_LOG_STAT_HEADER_SIZE;

    fort_stat_dpc_traf_flush(stat, proc_count, out);
  }

  /* Flush process group statistics */
  fort_stat_dpc_group_flush(stat);

  /* Unlock stat */
  fort_stat_dpc_end(&stat_lock_queue);

  /* Flush pending buffer */
  fort_buffer_dpc_flush_pending(buf, &irp, &info);

  /* Unlock buffer */
  fort_buffer_dpc_end(&buf_lock_queue);

  if (irp != NULL) {
    fort_request_complete_info(irp, STATUS_SUCCESS, info);
  }

  /* Flush deferred packets */
  fort_callout_defer_flush(TRUE);
}

static NTSTATUS
fort_device_create (PDEVICE_OBJECT device, PIRP irp)
{
  NTSTATUS status = STATUS_SUCCESS;

  UNUSED(device);

  /* Device opened */
  {
    KLOCK_QUEUE_HANDLE lock_queue;

    KeAcquireInStackQueuedSpinLock(&g_device->conf_lock, &lock_queue);
    if (g_device->is_opened) {
      status = STATUS_SHARING_VIOLATION;  /* Only one client may connect */
    } else {
      g_device->is_opened = TRUE;
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
  }

  fort_request_complete(irp, status);

  return status;
}

static NTSTATUS
fort_device_close (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  fort_request_complete(irp, STATUS_SUCCESS);

  return STATUS_SUCCESS;
}

static NTSTATUS
fort_device_cleanup (PDEVICE_OBJECT device, PIRP irp)
{
  /* Clear conf */
  {
    const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(NULL);
    FORT_CONF_FLAGS conf_flags;

    RtlZeroMemory(&conf_flags, sizeof(FORT_CONF_FLAGS));
    conf_flags.prov_boot = g_device->prov_boot;

    fort_callout_force_reauth(device, old_conf_flags, conf_flags);
  }

  /* Clear buffer */
  fort_buffer_clear(&g_device->buffer);

  /* Device closed */
  {
    KLOCK_QUEUE_HANDLE lock_queue;

    KeAcquireInStackQueuedSpinLock(&g_device->conf_lock, &lock_queue);
    g_device->is_opened = FALSE;
    KeReleaseInStackQueuedSpinLock(&lock_queue);
  }

  fort_request_complete(irp, STATUS_SUCCESS);

  return STATUS_SUCCESS;
}

static void
fort_device_cancel_pending (PDEVICE_OBJECT device, PIRP irp)
{
  ULONG_PTR info;
  NTSTATUS status;

  UNUSED(device);

  status = fort_buffer_cancel_pending(&g_device->buffer, irp, &info);

  IoReleaseCancelSpinLock(irp->CancelIrql);  /* before IoCompleteRequest()! */

  fort_request_complete_info(irp, status, info);
}

static NTSTATUS
fort_device_control (PDEVICE_OBJECT device, PIRP irp)
{
  PIO_STACK_LOCATION irp_stack;
  ULONG_PTR info = 0;
  NTSTATUS status = STATUS_INVALID_PARAMETER;

  irp_stack = IoGetCurrentIrpStackLocation(irp);

  switch (irp_stack->Parameters.DeviceIoControl.IoControlCode) {
  case FORT_IOCTL_SETCONF: {
    const PFORT_CONF_IO conf_io = irp->AssociatedIrp.SystemBuffer;
    const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

    if (len > sizeof(FORT_CONF_IO)
        && conf_io->driver_version == DRIVER_VERSION) {
      const PFORT_CONF conf = &conf_io->conf;
      PFORT_CONF_REF conf_ref = fort_conf_ref_new(
        conf, len - FORT_CONF_IO_CONF_OFF);

      if (conf_ref == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
      } else {
        const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(conf_ref);
        const FORT_CONF_FLAGS conf_flags = conf_ref->conf.flags;

        fort_stat_update_limits(&g_device->stat, conf_io);

        status = fort_callout_force_reauth(device, old_conf_flags, conf_flags);
      }
    }
    break;
  }
  case FORT_IOCTL_SETFLAGS: {
    const PFORT_CONF_FLAGS conf_flags = irp->AssociatedIrp.SystemBuffer;
    const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

    if (len == sizeof(FORT_CONF_FLAGS)) {
      const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_flags_set(conf_flags);

      status = fort_callout_force_reauth(device, old_conf_flags, *conf_flags);
    }
    break;
  }
  case FORT_IOCTL_GETLOG: {
    PVOID out = irp->AssociatedIrp.SystemBuffer;
    const ULONG out_len = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;

    status = fort_buffer_xmove(&g_device->buffer, irp, out, out_len, &info);

    if (status == STATUS_PENDING) {
      KIRQL cirq;

      IoMarkIrpPending(irp);

      IoAcquireCancelSpinLock(&cirq);
      IoSetCancelRoutine(irp, fort_device_cancel_pending);
      IoReleaseCancelSpinLock(cirq);

      return STATUS_PENDING;
    }
    break;
  }
  default: break;
  }

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Device Control: Error: %x\n", status);
  }

  fort_request_complete_info(irp, status, info);

  return status;
}

static void
fort_power_callback (PVOID context, PVOID event, PVOID specifics)
{
  BOOL power_off;

  UNUSED(context);

  if (event != (PVOID) PO_CB_SYSTEM_STATE_LOCK)
    return;

  power_off = (specifics == NULL);
  g_device->power_off = power_off;

  if (power_off) {
    fort_callout_defer_flush(FALSE);
  }
}

static NTSTATUS
fort_power_callback_register (void)
{
  OBJECT_ATTRIBUTES obj_attr;
  UNICODE_STRING obj_name;
  NTSTATUS status;

  RtlInitUnicodeString(&obj_name, L"\\Callback\\PowerState");

  InitializeObjectAttributes(&obj_attr, &obj_name,
    OBJ_CASE_INSENSITIVE, NULL, NULL);

  status = ExCreateCallback(&g_device->power_cb_obj, &obj_attr, FALSE, TRUE);

  if (NT_SUCCESS(status)) {
    g_device->power_cb_reg = ExRegisterCallback(g_device->power_cb_obj,
      fort_power_callback, NULL);
  }

  return status;
}

static void
fort_power_callback_unregister (void)
{
  if (g_device->power_cb_reg != NULL) {
    ExUnregisterCallback(g_device->power_cb_reg);
  }

  if (g_device->power_cb_obj != NULL) {
    ObDereferenceObject(g_device->power_cb_obj);
  }
}

static void
fort_driver_unload (PDRIVER_OBJECT driver)
{
  UNICODE_STRING device_link;

  if (g_device != NULL) {
    fort_callout_defer_flush(FALSE);

    fort_timer_close(&g_device->timer);
    fort_defer_close(&g_device->defer);
    fort_stat_close(&g_device->stat);
    fort_buffer_close(&g_device->buffer);

    fort_power_callback_unregister();

    if (!g_device->prov_boot) {
      fort_prov_unregister(0);
    }

    fort_callout_remove();
  }

  RtlInitUnicodeString(&device_link, DOS_DEVICE_NAME);
  IoDeleteSymbolicLink(&device_link);

  IoDeleteDevice(driver->DeviceObject);
}

static NTSTATUS
fort_bfe_wait (void) {
  LARGE_INTEGER delay;
  delay.QuadPart = -5000000;  /* sleep 500000us (500ms) */
  int count = 600;  /* wait for 5 minutes */

  do {
    const FWPM_SERVICE_STATE state = FwpmBfeStateGet0();
    if (state == FWPM_SERVICE_RUNNING)
      return STATUS_SUCCESS;

    KeDelayExecutionThread(KernelMode, FALSE, &delay);
  } while (--count >= 0);

  return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
DriverEntry (PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
  UNICODE_STRING device_name;
  PDEVICE_OBJECT device;
  NTSTATUS status;

  UNUSED(reg_path);

  /* Wait for BFE to start */
  status = fort_bfe_wait();
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Entry: Error: BFE is not running\n");
    return status;
  }

  RtlInitUnicodeString(&device_name, NT_DEVICE_NAME);
  status = IoCreateDevice(driver, sizeof(FORT_DEVICE), &device_name,
                          FORT_DEVICE_TYPE, 0, FALSE, &device);

  if (NT_SUCCESS(status)) {
    UNICODE_STRING device_link;

    RtlInitUnicodeString(&device_link, DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&device_link, &device_name);

    if (NT_SUCCESS(status)) {
      driver->MajorFunction[IRP_MJ_CREATE] = fort_device_create;
      driver->MajorFunction[IRP_MJ_CLOSE] = fort_device_close;
      driver->MajorFunction[IRP_MJ_CLEANUP] = fort_device_cleanup;
      driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = fort_device_control;
      driver->DriverUnload = fort_driver_unload;

      device->Flags |= DO_BUFFERED_IO;

      g_device = device->DeviceExtension;

      RtlZeroMemory(g_device, sizeof(FORT_DEVICE));

      fort_buffer_open(&g_device->buffer);
      fort_stat_open(&g_device->stat);
      fort_defer_open(&g_device->defer);
      fort_timer_open(&g_device->timer, &fort_callout_timer);

      KeInitializeSpinLock(&g_device->conf_lock);

      /* Unegister old filters provider */
      {
        g_device->prov_boot = fort_prov_is_boot();

        fort_prov_unregister(0);
      }

      /* Install callouts */
      status = fort_callout_install(device);

      /* Register filters provider */
      if (NT_SUCCESS(status)) {
        status = fort_prov_register(0, g_device->prov_boot);
      }

      /* Register power state change callback */
      if (NT_SUCCESS(status)) {
        status = fort_power_callback_register();
      }
    }
  }

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Entry: Error: %x\n", status);
    fort_driver_unload(driver);
  }

  return status;
}
