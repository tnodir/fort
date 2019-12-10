/* Fort Firewall Driver */

#define NDIS_WDM	1
#define NDIS630		1

#define WIN9X_COMPAT_SPINLOCK  /* XXX: Support Windows 7: KeInitializeSpinLock() */
#define POOL_NX_OPTIN	1  /* Enhanced protection of NX pool */

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
#include "fortcnf.c"
#include "fortbuf.c"
#include "fortpkt.c"
#include "fortstat.c"
#include "forttmr.c"
#include "fortwrk.c"

typedef struct fort_device {
  UINT32 connect4_id;
  UINT32 accept4_id;

  PCALLBACK_OBJECT power_cb_obj;
  PVOID power_cb_reg;

  PCALLBACK_OBJECT systime_cb_obj;
  PVOID systime_cb_reg;

  FORT_DEVICE_CONF conf;
  FORT_BUFFER buffer;
  FORT_STAT stat;
  FORT_DEFER defer;
  FORT_TIMER log_timer;
  FORT_TIMER app_timer;
#ifdef LOG_HEARTBEAT
  FORT_TIMER heartbeat_timer;
  UINT16 volatile heartbeat_tick;
#endif
  FORT_WORKER worker;
} FORT_DEVICE, *PFORT_DEVICE;

static PFORT_DEVICE g_device = NULL;


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
                          int flagsField, int remoteIpField,
                          int remotePortField, int ipProtoField)
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

  if (!g_device->conf.conf_flags.filter_locals
      && ((flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
        || remote_ip == 0xFFFFFFFF)) {  /* Local broadcast */
    fort_callout_classify_permit(filter, classifyOut);
    return;
  }

  conf_ref = fort_conf_ref_take(&g_device->conf);

  if (conf_ref == NULL) {
    if (fort_device_flag(&g_device->conf, FORT_DEVICE_PROV_BOOT)) {
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
    const FORT_APP_FLAGS app_flags = fort_conf_app_find(
      &conf_ref->conf, path_len, path, fort_conf_ref_exe_find);

    if (!fort_conf_app_blocked(&conf_ref->conf, app_flags)) {
      if (conf_flags.log_stat) {
        const UINT64 flow_id = inMetaValues->flowHandle;

        const IPPROTO ip_proto = (IPPROTO) inFixedValues->incomingValue[
          ipProtoField].value.uint8;
        const BOOL is_tcp = (ip_proto == IPPROTO_TCP);

        const UCHAR group_index = app_flags.group_index;
        const BOOL is_reauth = (flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE);

        BOOL is_new_proc = FALSE;
        NTSTATUS status;

        status = fort_flow_associate(&g_device->stat, flow_id, process_id,
          group_index, is_tcp, is_reauth, &is_new_proc);

        if (!NT_SUCCESS(status)) {
          if (status == FORT_STATUS_FLOW_BLOCK)
            goto block;

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
    const UINT16 remote_port = inFixedValues->incomingValue[
      remotePortField].value.uint16;
    const IPPROTO ip_proto = (IPPROTO) inFixedValues->incomingValue[
      ipProtoField].value.uint8;

    fort_buffer_blocked_write(&g_device->buffer,
      remote_ip, remote_port, ip_proto,
      process_id, path_len, path, &irp, &info);
  }

 block:
  fort_callout_classify_block(classifyOut);
  goto end;

 permit:
  fort_callout_classify_permit(filter, classifyOut);

 end:
  fort_conf_ref_put(&g_device->conf, conf_ref);

  if (irp != NULL) {
    fort_request_complete_info(irp, STATUS_SUCCESS, info);
  }
}

static void
fort_callout_connect_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                         const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                         void *layerData,
                         const FWPS_FILTER0 *filter,
                         const UINT64 flowContext,
                         FWPS_CLASSIFY_OUT0 *classifyOut)
{
  UNUSED(layerData);
  UNUSED(flowContext);

  fort_callout_classify_v4(inFixedValues, inMetaValues, filter, classifyOut,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL);
}

static void
fort_callout_accept_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                        void *layerData,
                        const FWPS_FILTER0 *filter,
                        const UINT64 flowContext,
                        FWPS_CLASSIFY_OUT0 *classifyOut)
{
  UNUSED(layerData);
  UNUSED(flowContext);

  fort_callout_classify_v4(inFixedValues, inMetaValues, filter, classifyOut,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL);
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
fort_packet_inject_complete (PFORT_PACKET pkt,
                             PNET_BUFFER_LIST clonedNetBufList,
                             BOOLEAN dispatchLevel)
{
  fort_defer_packet_free(&g_device->defer, pkt, clonedNetBufList, dispatchLevel);
}

static void
fort_callout_defer_packet_flush (UINT32 list_bits,
                                 BOOL dispatchLevel)
{
  fort_defer_packet_flush(&g_device->defer, fort_packet_inject_complete,
                          list_bits, dispatchLevel);
}

static void
fort_callout_defer_stream_flush (UINT64 flow_id,
                                 BOOL dispatchLevel)
{
  fort_defer_stream_flush(&g_device->defer, fort_packet_inject_complete,
                          flow_id, FALSE);
}

static void
fort_callout_defer_flush (void)
{
  fort_callout_defer_packet_flush(FORT_DEFER_FLUSH_ALL, FALSE);
  fort_callout_defer_stream_flush(FORT_DEFER_STREAM_ALL, FALSE);
}

static void
fort_callout_flow_classify_v4 (const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                               UINT64 flowContext,
                               FWPS_CLASSIFY_OUT0 *classifyOut,
                               UINT32 dataSize, BOOL is_tcp, BOOL inbound)
{
  const UINT32 headerSize = inbound ? inMetaValues->transportHeaderSize : 0;

  fort_flow_classify(&g_device->stat, flowContext,
    headerSize + dataSize, is_tcp, inbound);
}

static void
fort_callout_stream_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                                 const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                                 FWPS_STREAM_CALLOUT_IO_PACKET0 *packet,
                                 const FWPS_FILTER0 *filter,
                                 UINT64 flowContext,
                                 FWPS_CLASSIFY_OUT0 *classifyOut)
{
  const FWPS_STREAM_DATA0 *streamData = packet->streamData;
  const UINT32 streamFlags = streamData->flags;
  const UINT32 dataSize = (UINT32) streamData->dataLength;

  const BOOL inbound = (streamFlags & FWPS_STREAM_FLAG_RECEIVE) != 0;

  UNUSED(inFixedValues);

  fort_callout_flow_classify_v4(inMetaValues, flowContext,
    classifyOut, dataSize, TRUE, inbound);

  /* Flush flow's deferred TCP packets on FIN */
  #if 0
  if (streamFlags & (FWPS_STREAM_FLAG_RECEIVE_DISCONNECT
      | FWPS_STREAM_FLAG_SEND_DISCONNECT)) {
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);

    if (flow_flags & FORT_FLOW_SPEED_LIMIT) {
      fort_callout_defer_packet_flush(flow->flow_id, FORT_DEFER_FLUSH_ALL, FALSE);
    }

    if (flow_flags & FORT_FLOW_FRAGMENT) {
      fort_callout_defer_stream_flush(flow->flow_id, FALSE);
    }

    goto permit;
  }
  #endif

  /* Fragment first TCP packet */
  if ((streamFlags & (FWPS_STREAM_FLAG_SEND
        | FWPS_STREAM_FLAG_SEND_EXPEDITED
        | FWPS_STREAM_FLAG_SEND_DISCONNECT))
      == FWPS_STREAM_FLAG_SEND) {
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);

    const UCHAR fragment_flags = (flow_flags
        & (FORT_FLOW_FRAGMENT | FORT_FLOW_FRAGMENT_DEFER | FORT_FLOW_FRAGMENTED));

    if (fragment_flags != 0
        && !(fragment_flags & FORT_FLOW_FRAGMENTED)) {
      const UCHAR fragment_size = 3;

      if (fragment_flags & FORT_FLOW_FRAGMENT_DEFER) {
        const NTSTATUS status = fort_defer_stream_add(&g_device->defer,
          inFixedValues, inMetaValues, streamData, filter, inbound);

        if (NT_SUCCESS(status))
          goto drop;

        fort_flow_flags_set(flow, FORT_FLOW_FRAGMENTED, TRUE);
      }
      else if (dataSize > fragment_size) {
        packet->countBytesEnforced = fragment_size;

        fort_flow_flags_set(flow, FORT_FLOW_FRAGMENT_DEFER, TRUE);
      }
    }
  }

 /* permit: */
  fort_callout_classify_permit(filter, classifyOut);
  return;

 drop:
  fort_callout_classify_drop(classifyOut);
  return;
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

  fort_callout_flow_classify_v4(inMetaValues, flowContext,
    classifyOut, dataSize, FALSE, inbound);

  fort_callout_classify_permit(filter, classifyOut);
}

static void
fort_callout_flow_delete_v4 (UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
  UNUSED(layerId);
  UNUSED(calloutId);

  fort_flow_delete(&g_device->stat, flowContext);
}

static void
fort_callout_transport_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                                    const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                                    PNET_BUFFER_LIST netBufList,
                                    const FWPS_FILTER0 *filter,
                                    UINT64 flowContext,
                                    FWPS_CLASSIFY_OUT0 *classifyOut,
                                    BOOL inbound)
{
  PNET_BUFFER netBuf;

  if (!FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues,
        FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED)
      && netBufList != NULL
      && (netBuf = NET_BUFFER_LIST_FIRST_NB(netBufList)) != NULL) {
    PFORT_FLOW flow = (PFORT_FLOW) flowContext;

    const UCHAR flow_flags = fort_flow_flags(flow);

    const UCHAR defer_flag = inbound
      ? FORT_FLOW_DEFER_IN : FORT_FLOW_DEFER_OUT;
    const UCHAR speed_limit = inbound
      ? FORT_FLOW_SPEED_LIMIT_OUT : FORT_FLOW_SPEED_LIMIT_IN;

    const UCHAR speed_defer_flags = speed_limit | defer_flag;
    const BOOL defer_flow = (flow_flags & speed_defer_flags) == speed_defer_flags
      && !fort_device_flag(&g_device->conf, FORT_DEVICE_POWER_OFF);

    const BOOL fragment_packet = !inbound && (flow_flags
      & (FORT_FLOW_FRAGMENT_DEFER | FORT_FLOW_FRAGMENTED))
      == FORT_FLOW_FRAGMENT_DEFER;

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

    /* Defer TCP Pure (zero length) ACK-packets */
    if (defer_flow && NET_BUFFER_DATA_LENGTH(netBuf) == headerOffset) {
      const NTSTATUS status = fort_defer_packet_add(&g_device->defer,
        inFixedValues, inMetaValues, netBufList, inbound,
        flow->opt.group_index);

      if (NT_SUCCESS(status))
        goto drop;

      if (status == STATUS_CANT_TERMINATE_SELF) {
        /* Clear ACK deferring */
        fort_flow_flags_set(flow, defer_flag, FALSE);
      }

      goto permit;
    }

    /* Fragment first TCP packet */
    if (fragment_packet) {
      fort_defer_stream_flush(&g_device->defer, fort_packet_inject_complete,
        flow->flow_id, FALSE);

      fort_flow_flags_set(flow, FORT_FLOW_FRAGMENTED, TRUE);
    }
  }

 permit:
  fort_callout_classify_permit(filter, classifyOut);
  return;

 drop:
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
    filter, flowContext, classifyOut, TRUE);
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
    filter, flowContext, classifyOut, FALSE);
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

  /* reuse c.flowDeleteFn & c.flags */

  status = FwpsCalloutRegister0(device, &c, &g_device->stat.datagram4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Register Datagram V4: Error: %x\n", status);
    return status;
  }

  /* IPv4 inbound transport callout */
  c.calloutKey = FORT_GUID_CALLOUT_IN_TRANSPORT_V4;
  c.classifyFn = fort_callout_in_transport_classify_v4;

  c.flowDeleteFn = fort_callout_delete_v4;
  /* reuse c.flags */

  status = FwpsCalloutRegister0(device, &c, &g_device->stat.in_transport4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Register Inbound Transport V4: Error: %x\n", status);
    return status;
  }

  /* IPv4 outbound transport callout */
  c.calloutKey = FORT_GUID_CALLOUT_OUT_TRANSPORT_V4;
  c.classifyFn = fort_callout_out_transport_classify_v4;

  /* reuse c.flowDeleteFn & c.flags */

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

static NTSTATUS
fort_callout_force_reauth (const FORT_CONF_FLAGS old_conf_flags,
                           UINT32 defer_flush_bits)
{
  PFORT_STAT stat = &g_device->stat;
  FORT_CONF_FLAGS conf_flags;

  HANDLE engine;
  NTSTATUS status;

  fort_timer_update(&g_device->log_timer, FALSE);

  /* Check app group periods & update group_bits */
  {
    int periods_n = 0;

    fort_conf_ref_period_update(&g_device->conf, TRUE, &periods_n);

    fort_timer_update(&g_device->app_timer,
      (periods_n != 0));
  }

  conf_flags = g_device->conf.conf_flags;

  /* Handle log_stat */
  if (old_conf_flags.log_stat != conf_flags.log_stat) {
    fort_stat_update(stat, conf_flags.log_stat);

    if (!conf_flags.log_stat) {
      defer_flush_bits = FORT_DEFER_FLUSH_ALL;
    }
  }

  if (defer_flush_bits != 0) {
    fort_callout_defer_packet_flush(defer_flush_bits, FALSE);
  }

  /* Open provider */
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
  {
    const PFORT_CONF_GROUP conf_group = &stat->conf_group;
    const UINT16 filter_bits = (conf_group->fragment_bits | conf_group->limit_bits);

    const BOOL old_filter_transport = fort_device_flag(
      &g_device->conf, FORT_DEVICE_FILTER_TRANSPORT) != 0;
    const BOOL filter_transport = (conf_flags.group_bits & filter_bits) != 0;

    if (old_conf_flags.log_stat != conf_flags.log_stat
        || old_filter_transport != filter_transport) {
      fort_device_flag_set(&g_device->conf, FORT_DEVICE_FILTER_TRANSPORT, filter_transport);

      fort_prov_flow_unregister(engine);

 stat_prov:
      if (conf_flags.log_stat) {
        if ((status = fort_prov_flow_register(engine, filter_transport)))
          goto cleanup;
      }
    }
  }

  /* Force reauth filter */
  if ((status = fort_prov_reauth(engine)))
    goto cleanup;

  fort_timer_update(&g_device->log_timer,
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

  UINT32 defer_flush_bits;

  /* Lock buffer */
  fort_buffer_dpc_begin(buf, &buf_lock_queue);

  /* Lock stat */
  fort_stat_dpc_begin(stat, &stat_lock_queue);

  /* Flush traffic statistics */
  while (stat->proc_active_count != 0) {
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
  defer_flush_bits = fort_stat_dpc_group_flush(stat);

  /* Unlock stat */
  fort_stat_dpc_end(&stat_lock_queue);

  /* Flush pending buffer */
  if (irp == NULL) {
    fort_buffer_dpc_flush_pending(buf, &irp, &info);
  }

  /* Unlock buffer */
  fort_buffer_dpc_end(&buf_lock_queue);

  if (irp != NULL) {
    fort_request_complete_info(irp, STATUS_SUCCESS, info);
  }

  /* Flush deferred packets */
  fort_callout_defer_packet_flush(defer_flush_bits, TRUE);
}

static void
fort_worker_reauth (void)
{
  const FORT_CONF_FLAGS conf_flags = g_device->conf.conf_flags;
  NTSTATUS status;

  status = fort_callout_force_reauth(conf_flags, 0);

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Worker Reauth: Error: %x\n", status);
  }
}

static void
fort_app_period_timer (void)
{
  if (fort_conf_ref_period_update(&g_device->conf, FALSE, NULL)) {
    fort_worker_queue(&g_device->worker,
      FORT_WORKER_REAUTH, &fort_worker_reauth);
  }
}

#ifdef LOG_HEARTBEAT
static void
fort_heartbeat_timer (void)
{
  PFORT_BUFFER buf = &g_device->buffer;
  KLOCK_QUEUE_HANDLE buf_lock_queue;

  /* Lock buffer */
  KeAcquireInStackQueuedSpinLock(&buf->lock, &buf_lock_queue);

  /* Log heartbeat */
  {
    const UINT16 tick = InterlockedIncrement16(&g_device->heartbeat_tick);
    const UINT32 len = FORT_LOG_HEARTBEAT_SIZE;
    PCHAR out;
    NTSTATUS status;

    status = fort_buffer_prepare(buf, len, &out, NULL, NULL);
    if (!NT_SUCCESS(status)) {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                 "FORT: Heartbeat Timer: Error: %x\n", status);
      goto end;
    }

    fort_log_heartbeat_write(out, tick);
  }

 end:
  /* Unlock buffer */
  KeReleaseInStackQueuedSpinLock(&buf_lock_queue);
}
#endif

static NTSTATUS
fort_device_create (PDEVICE_OBJECT device, PIRP irp)
{
  NTSTATUS status = STATUS_SUCCESS;

  UNUSED(device);

  /* Device opened */
  if (fort_device_flag_set(&g_device->conf, FORT_DEVICE_IS_OPENED, TRUE)
      & FORT_DEVICE_IS_OPENED) {
    status = STATUS_SHARING_VIOLATION;  /* Only one client may connect */
  }

  if (NT_SUCCESS(status)) {
    /* Clear buffer */
    fort_buffer_clear(&g_device->buffer);

#ifdef LOG_HEARTBEAT
    InterlockedAnd16(&g_device->heartbeat_tick, 0);
    fort_heartbeat_timer();
    fort_timer_update(&g_device->heartbeat_timer, TRUE);
#endif
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
  /* Device closed */
  fort_device_flag_set(&g_device->conf, FORT_DEVICE_IS_OPENED, FALSE);

  /* Clear conf */
  {
    const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(
      &g_device->conf, NULL);

    fort_callout_force_reauth(old_conf_flags, FORT_DEFER_FLUSH_ALL);
  }

#ifdef LOG_HEARTBEAT
  fort_timer_update(&g_device->heartbeat_timer, FALSE);
  fort_heartbeat_timer();
#endif

  /* Clear buffer */
  fort_buffer_clear(&g_device->buffer);

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
  case FORT_IOCTL_VALIDATE: {
    const PFORT_CONF_VERSION conf_ver = irp->AssociatedIrp.SystemBuffer;
    const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

    if (len == sizeof(FORT_CONF_VERSION)) {
      status = (conf_ver->driver_version == DRIVER_VERSION)
        ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }
    break;
  }
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
        PFORT_STAT stat = &g_device->stat;

        const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(
          &g_device->conf, conf_ref);

        const UINT32 defer_flush_bits =
          (stat->conf_group.limit_2bits ^ conf_io->conf_group.limit_2bits);

        fort_stat_conf_update(stat, conf_io);

        status = fort_callout_force_reauth(old_conf_flags, defer_flush_bits);
      }
    }
    break;
  }
  case FORT_IOCTL_SETFLAGS: {
    const PFORT_CONF_FLAGS conf_flags = irp->AssociatedIrp.SystemBuffer;
    const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

    if (len == sizeof(FORT_CONF_FLAGS)) {
      const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_flags_set(
        &g_device->conf, conf_flags);

      const UINT32 defer_flush_bits =
        (old_conf_flags.group_bits != conf_flags->group_bits ? FORT_DEFER_FLUSH_ALL : 0);

      status = fort_callout_force_reauth(old_conf_flags, defer_flush_bits);
    }
    break;
  }
  case FORT_IOCTL_GETLOG: {
    PVOID out = irp->AssociatedIrp.SystemBuffer;
    const ULONG out_len = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;

    if (out_len < FORT_BUFFER_SIZE) {
      status = STATUS_BUFFER_TOO_SMALL;
    } else {
      status = fort_buffer_xmove(&g_device->buffer, irp, out, out_len, &info);

      if (status == STATUS_PENDING) {
        KIRQL cirq;

        IoMarkIrpPending(irp);

        IoAcquireCancelSpinLock(&cirq);
        IoSetCancelRoutine(irp, fort_device_cancel_pending);
        IoReleaseCancelSpinLock(cirq);

        return STATUS_PENDING;
      }
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

  fort_device_flag_set(&g_device->conf, FORT_DEVICE_POWER_OFF, power_off);

  if (power_off) {
    fort_callout_defer_flush();
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
fort_systime_callback (PVOID context, PVOID event, PVOID specifics)
{
  UNUSED(context);
  UNUSED(event);
  UNUSED(specifics);

  if (g_device->app_timer.running) {
    fort_app_period_timer();
  }
}

static NTSTATUS
fort_systime_callback_register (void)
{
  OBJECT_ATTRIBUTES obj_attr;
  UNICODE_STRING obj_name;
  NTSTATUS status;

  RtlInitUnicodeString(&obj_name, L"\\Callback\\SetSystemTime");

  InitializeObjectAttributes(&obj_attr, &obj_name,
    OBJ_CASE_INSENSITIVE, NULL, NULL);

  status = ExCreateCallback(&g_device->systime_cb_obj, &obj_attr, FALSE, TRUE);

  if (NT_SUCCESS(status)) {
    g_device->systime_cb_reg = ExRegisterCallback(g_device->systime_cb_obj,
      fort_systime_callback, NULL);
  }

  return status;
}

static void
fort_systime_callback_unregister (void)
{
  if (g_device->systime_cb_reg != NULL) {
    ExUnregisterCallback(g_device->systime_cb_reg);
  }

  if (g_device->systime_cb_obj != NULL) {
    ObDereferenceObject(g_device->systime_cb_obj);
  }
}

static void
fort_driver_unload (PDRIVER_OBJECT driver)
{
  UNICODE_STRING device_link;

  if (g_device != NULL) {
    fort_callout_defer_flush();

#ifdef LOG_HEARTBEAT
    fort_timer_close(&g_device->heartbeat_timer);
#endif
    fort_timer_close(&g_device->app_timer);
    fort_timer_close(&g_device->log_timer);
    fort_defer_close(&g_device->defer);
    fort_stat_close(&g_device->stat);
    fort_buffer_close(&g_device->buffer);

    fort_worker_unregister(&g_device->worker);

    fort_power_callback_unregister();
    fort_systime_callback_unregister();

    if (!fort_device_flag(&g_device->conf, FORT_DEVICE_PROV_BOOT)) {
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

  // Use NX Non-Paged Pool
  ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

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

      fort_device_conf_open(&g_device->conf);
      fort_buffer_open(&g_device->buffer);
      fort_stat_open(&g_device->stat);
      fort_defer_open(&g_device->defer);
      fort_timer_open(&g_device->log_timer, 500, FALSE, &fort_callout_timer);
      fort_timer_open(&g_device->app_timer, 60000, TRUE, &fort_app_period_timer);
#ifdef LOG_HEARTBEAT
      fort_timer_open(&g_device->heartbeat_timer, 1000, TRUE, &fort_heartbeat_timer);
#endif

      /* Unregister old filters provider */
      {
        fort_device_flag_set(&g_device->conf, FORT_DEVICE_PROV_BOOT, fort_prov_is_boot());

        fort_prov_unregister(0);
      }

      /* Install callouts */
      status = fort_callout_install(device);

      /* Register worker */
      if (NT_SUCCESS(status)) {
        status = fort_worker_register(device, &g_device->worker);
      }

      /* Register filters provider */
      if (NT_SUCCESS(status)) {
        const BOOL prov_boot = fort_device_flag(&g_device->conf, FORT_DEVICE_PROV_BOOT);

        status = fort_prov_register(0, prov_boot);
      }

      /* Register power state change callback */
      if (NT_SUCCESS(status)) {
        status = fort_power_callback_register();
      }

      /* Register system time change callback */
      if (NT_SUCCESS(status)) {
        status = fort_systime_callback_register();
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
