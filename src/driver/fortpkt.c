/* Fort Firewall ACK Packets Deferring & Re-injection */

#define FORT_DEFER_FLUSH_ALL	0xFFFFFFFF
#define FORT_DEFER_LIST_MAX	(FORT_CONF_GROUP_MAX * 2)

#define FORT_DEFER_STREAM_ALL	((UINT64) ((INT64) -1))

#define HTONL(l)	_byteswap_ulong(l)
#define NTOHL(l)	HTONL(l)
#define HTONS(s)	_byteswap_ushort(s)
#define NTOHS(s)	HTONS(s)

#define TCP_FLAG_FIN	0x0001
#define TCP_FLAG_SYN	0x0002
#define TCP_FLAG_RST	0x0004
#define TCP_FLAG_PSH	0x0008
#define TCP_FLAG_ACK	0x0010
#define TCP_FLAG_URG	0x0020
#define TCP_FLAG_ECE	0x0040
#define TCP_FLAG_CWR	0x0080

typedef struct tcp_header {
  UINT16 source;	/* Source Port */
  UINT16 dest;		/* Destination Port */

  UINT32 seq;		/* Sequence number */
  UINT32 ack_seq;	/* Acknowledgement number */

  UCHAR res1	: 4;	/* Unused */
  UCHAR doff	: 4;	/* Data offset */

  UCHAR flags;		/* Flags */

  UINT16 window;	/* Window size */
  UINT16 csum;		/* Checksum */
  UINT16 urg_ptr;	/* Urgent Pointer */
} TCP_HEADER, *PTCP_HEADER;

typedef struct fort_packet_in {
  COMPARTMENT_ID compartmentId;

  IF_INDEX interfaceIndex;
  IF_INDEX subInterfaceIndex;

  UINT16 ipHeaderSize;
  UINT16 transportHeaderSize;
} FORT_PACKET_IN, *PFORT_PACKET_IN;

typedef struct fort_packet_out {
  COMPARTMENT_ID compartmentId;

  UINT32 remoteAddr4;
  SCOPE_ID remoteScopeId;

  UINT64 endpointHandle;
} FORT_PACKET_OUT, *PFORT_PACKET_OUT;

typedef struct fort_packet_stream {
  UINT16 layerId;

  UINT32 streamFlags;
  UINT32 calloutId;

  UINT64 flow_id;
} FORT_PACKET_STREAM, *PFORT_PACKET_STREAM;

typedef struct fort_packet {
  UINT32 inbound	: 1;
  UINT32 is_stream	: 1;
  UINT32 dataOffset	: 12;
  UINT32 dataSize	: 18;

  PNET_BUFFER_LIST netBufList;

  struct fort_packet *next;

  union {
    FORT_PACKET_IN in;
    FORT_PACKET_OUT out;
    FORT_PACKET_STREAM stream;
  };
} FORT_PACKET, *PFORT_PACKET;

typedef struct fort_defer_list {
  PFORT_PACKET packet_head;
  PFORT_PACKET packet_tail;
} FORT_DEFER_LIST, *PFORT_DEFER_LIST;

typedef struct fort_defer {
  UINT32 list_bits;

  HANDLE transport_injection4_id;
  HANDLE stream_injection4_id;

  PFORT_PACKET packet_free;

  tommy_arrayof packets;

  FORT_DEFER_LIST stream_list;
  FORT_DEFER_LIST lists[FORT_DEFER_LIST_MAX];  /* in/out-bounds */

  KSPIN_LOCK lock;
} FORT_DEFER, *PFORT_DEFER;

typedef void (*FORT_INJECT_COMPLETE_FUNC) (PFORT_PACKET, PNET_BUFFER_LIST, BOOLEAN);
typedef NTSTATUS (*FORT_INJECT_FUNC) (PFORT_DEFER, PFORT_PACKET, PNET_BUFFER_LIST *, FORT_INJECT_COMPLETE_FUNC);


static NTSTATUS
fort_packet_inject_in (PFORT_DEFER defer,
                       PFORT_PACKET pkt,
                       PNET_BUFFER_LIST *clonedNetBufList,
                       FORT_INJECT_COMPLETE_FUNC complete_func)
{
  PFORT_PACKET_IN pkt_in = &pkt->in;
  PNET_BUFFER_LIST netBufList = pkt->netBufList;
  PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(netBufList);
  NTSTATUS status;

  /* The TCP/IP stack could have retreated the net buffer list by the 
   * transportHeaderSize amount; detect the condition here to avoid
   * retreating twice.
   */
  if (pkt->dataOffset != NET_BUFFER_DATA_OFFSET(netBuffer)) {
     pkt_in->transportHeaderSize = 0;
  }

  /* Adjust the net buffer list offset to the start of the IP header. */
  NdisRetreatNetBufferDataStart(netBuffer,
    pkt_in->ipHeaderSize + pkt_in->transportHeaderSize, 0, NULL);

  status = FwpsAllocateCloneNetBufferList0(netBufList,
    NULL, NULL, 0, clonedNetBufList);

  /* Undo the adjustment on the original net buffer list. */
  NdisAdvanceNetBufferDataStart(netBuffer,
    pkt_in->ipHeaderSize + pkt_in->transportHeaderSize, FALSE, NULL);

  if (NT_SUCCESS(status)) {
    status = FwpsInjectTransportReceiveAsync0(
      defer->transport_injection4_id, NULL, NULL, 0,
      AF_INET, pkt_in->compartmentId,
      pkt_in->interfaceIndex, pkt_in->subInterfaceIndex,
      *clonedNetBufList, complete_func, pkt);
  }

  return status;
}

static NTSTATUS
fort_packet_inject_out (PFORT_DEFER defer,
                        PFORT_PACKET pkt,
                        PNET_BUFFER_LIST *clonedNetBufList,
                        FORT_INJECT_COMPLETE_FUNC complete_func)
{
  PFORT_PACKET_OUT pkt_out = &pkt->out;
  PNET_BUFFER_LIST netBufList = pkt->netBufList;
  NTSTATUS status;

  status = FwpsAllocateCloneNetBufferList0(netBufList,
    NULL, NULL, 0, clonedNetBufList);

  if (NT_SUCCESS(status)) {
    FWPS_TRANSPORT_SEND_PARAMS0 send_args;

    RtlZeroMemory(&send_args, sizeof(FWPS_TRANSPORT_SEND_PARAMS0));
    send_args.remoteAddress = (UCHAR *) &pkt_out->remoteAddr4;
    send_args.remoteScopeId = pkt_out->remoteScopeId;

    status = FwpsInjectTransportSendAsync0(
      defer->transport_injection4_id, NULL,
      pkt_out->endpointHandle, 0,
      &send_args, AF_INET, pkt_out->compartmentId,
      *clonedNetBufList, complete_func, pkt);
  }

  return status;
}

static NTSTATUS
fort_packet_inject_stream (PFORT_DEFER defer,
                           PFORT_PACKET pkt,
                           PNET_BUFFER_LIST *clonedNetBufList,
                           FORT_INJECT_COMPLETE_FUNC complete_func)
{
  PFORT_PACKET_STREAM pkt_stream = &pkt->stream;
  PNET_BUFFER_LIST netBufList = pkt->netBufList;
  PNET_BUFFER netBuffer = NET_BUFFER_LIST_FIRST_NB(netBufList);
  NTSTATUS status;

  /* Adjust the net buffer list offset to the start of the actual data. */
  NdisAdvanceNetBufferDataStart(netBuffer, pkt->dataOffset, FALSE, NULL);

  status = FwpsAllocateCloneNetBufferList0(netBufList,
    NULL, NULL, 0, clonedNetBufList);

  /* Undo the adjustment on the original net buffer list. */
  NdisRetreatNetBufferDataStart(netBuffer, pkt->dataOffset, 0, NULL);

  if (NT_SUCCESS(status)) {
    status = FwpsStreamInjectAsync0(
      defer->stream_injection4_id, NULL, 0,
      pkt_stream->flow_id, pkt_stream->calloutId,
      pkt_stream->layerId, pkt_stream->streamFlags,
      *clonedNetBufList, pkt->dataSize,
      complete_func, pkt);
  }

  return status;
}

static BOOL
fort_packet_injected_by_self (PFORT_DEFER defer,
                              PNET_BUFFER_LIST netBufList)
{
  const FWPS_PACKET_INJECTION_STATE state = FwpsQueryPacketInjectionState0(
    defer->transport_injection4_id, netBufList, NULL);

  return (state == FWPS_PACKET_INJECTED_BY_SELF
    || state == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF);
}

static PFORT_PACKET
fort_defer_packet_get (PFORT_DEFER defer)
{
  PFORT_PACKET pkt = NULL;

  if (defer->packet_free != NULL) {
    pkt = defer->packet_free;
    defer->packet_free = pkt->next;
  } else {
    const tommy_count_t size = tommy_arrayof_size(&defer->packets);

    /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
    tommy_arrayof_grow(&defer->packets, size + 1);

    pkt = tommy_arrayof_ref(&defer->packets, size);
  }

  return pkt;
}

static void
fort_defer_packet_put (PFORT_DEFER defer, PFORT_PACKET pkt)
{
  pkt->next = defer->packet_free;
  defer->packet_free = pkt;
}

static void
fort_defer_open (PFORT_DEFER defer)
{
  NTSTATUS status;

  status = FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_TRANSPORT,
    &defer->transport_injection4_id);

  if (!NT_SUCCESS(status)) {
    defer->transport_injection4_id = INVALID_HANDLE_VALUE;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Defer: Transport injection init error: %x\n", status);
  }

  status = FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_STREAM,
    &defer->stream_injection4_id);

  if (!NT_SUCCESS(status)) {
    defer->stream_injection4_id = INVALID_HANDLE_VALUE;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Defer: Stream injection init error: %x\n", status);
  }

  tommy_arrayof_init(&defer->packets, sizeof(FORT_PACKET));

  KeInitializeSpinLock(&defer->lock);
}

static void
fort_defer_close (PFORT_DEFER defer)
{
  FwpsInjectionHandleDestroy0(defer->transport_injection4_id);
  FwpsInjectionHandleDestroy0(defer->stream_injection4_id);

  tommy_arrayof_done(&defer->packets);
}

static void
fort_defer_list_add (PFORT_DEFER_LIST defer_list,
                     PFORT_PACKET pkt)
{
  if (defer_list->packet_tail == NULL) {
    defer_list->packet_head = defer_list->packet_tail = pkt;
  } else {
    defer_list->packet_tail->next = pkt;
    defer_list->packet_tail = pkt;
  }
}

static PFORT_PACKET
fort_defer_list_get (PFORT_DEFER_LIST defer_list,
                     PFORT_PACKET pkt_chain)
{
  if (defer_list->packet_head != NULL) {
    defer_list->packet_tail->next = pkt_chain;
    pkt_chain = defer_list->packet_head;

    defer_list->packet_head = defer_list->packet_tail = NULL;
  }

  return pkt_chain;
}

static PFORT_PACKET
fort_defer_list_flow_get (PFORT_DEFER_LIST defer_list,
                          PFORT_PACKET pkt_chain,
                          UINT64 flow_id)
{
  if (flow_id == FORT_DEFER_STREAM_ALL) {
    return fort_defer_list_get(defer_list, pkt_chain);
  }

  if (defer_list->packet_head != NULL) {
    PFORT_PACKET pkt_tail = pkt_chain;
    PFORT_PACKET pkt_prev = NULL;
    PFORT_PACKET pkt = defer_list->packet_head;

    do {
      PFORT_PACKET pkt_next = pkt->next;

      if (flow_id == pkt->stream.flow_id) {
        if (pkt_prev == NULL) {
          pkt_chain = pkt;
        } else {
          pkt_prev->next = pkt;
        }

        pkt_prev = pkt;
        pkt->next = pkt_tail;

        if (pkt == defer_list->packet_head) {
          defer_list->packet_head = pkt_next;
        }

        if (pkt == defer_list->packet_tail) {
          defer_list->packet_tail = NULL;
          break;
        }
      }

      pkt = pkt_next;
    } while (pkt != NULL);
  }

  return pkt_chain;
}

static NTSTATUS
fort_defer_packet_add (PFORT_DEFER defer,
                       const FWPS_INCOMING_VALUES0 *inFixedValues,
                       const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                       PNET_BUFFER_LIST netBufList,
                       BOOL inbound, UCHAR group_index)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_DEFER_LIST defer_list;
  PFORT_PACKET pkt;
  UINT16 list_index;
  NTSTATUS status;

  if (defer->transport_injection4_id == INVALID_HANDLE_VALUE)
    return STATUS_FWP_TCPIP_NOT_READY;

  /* Skip self injected packet */
  if (fort_packet_injected_by_self(defer, netBufList))
    return STATUS_CANT_TERMINATE_SELF;

  /* Skip IpSec protected packet */
  if (inbound) {
    FWPS_PACKET_LIST_INFORMATION info;

    status = FwpsGetPacketListSecurityInformation0(netBufList,
      FWPS_PACKET_LIST_INFORMATION_QUERY_IPSEC
      | FWPS_PACKET_LIST_INFORMATION_QUERY_INBOUND, &info);

    if (!NT_SUCCESS(status))
      return status;

    if (info.ipsecInformation.inbound.isSecure)
      return STATUS_UNSUCCESSFUL;
  }

  list_index = group_index * 2 + (inbound ? 0 : 1);

  KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);

  defer_list = &defer->lists[list_index];

  pkt = fort_defer_packet_get(defer);
  if (pkt == NULL) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto end;
  }

  fort_defer_list_add(defer_list, pkt);

  pkt->inbound = inbound;
  pkt->is_stream = FALSE;
  pkt->dataOffset = 0;
  pkt->dataSize = 0;  /* not used */
  pkt->netBufList = netBufList;
  pkt->next = NULL;

  if (inbound) {
    PFORT_PACKET_IN pkt_in = &pkt->in;

    const int interfaceField = inbound
      ? FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_INDEX
      : FWPS_FIELD_OUTBOUND_TRANSPORT_V4_INTERFACE_INDEX;

    const int subInterfaceField = inbound
      ? FWPS_FIELD_INBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX
      : FWPS_FIELD_OUTBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX;

    pkt_in->compartmentId = inMetaValues->compartmentId;

    pkt_in->interfaceIndex = inFixedValues->incomingValue[interfaceField].value.uint32;
    pkt_in->subInterfaceIndex = inFixedValues->incomingValue[subInterfaceField].value.uint32;

    pkt_in->ipHeaderSize = (UINT16) inMetaValues->ipHeaderSize;
    pkt_in->transportHeaderSize = (UINT16) inMetaValues->transportHeaderSize;

    pkt->dataOffset = (UINT16) NET_BUFFER_DATA_OFFSET(
      NET_BUFFER_LIST_FIRST_NB(netBufList));
  } else {
    PFORT_PACKET_OUT pkt_out = &pkt->out;

    const int remoteAddrField = inbound
      ? FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS
      : FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS;

    pkt_out->compartmentId = inMetaValues->compartmentId;

    /* host-order -> network-order conversion */
    pkt_out->remoteAddr4 = HTONL(
      inFixedValues->incomingValue[remoteAddrField].value.uint32);

    pkt_out->remoteScopeId = inMetaValues->remoteScopeId;
    pkt_out->endpointHandle = inMetaValues->transportEndpointHandle;
  }

  FwpsReferenceNetBufferList0(pkt->netBufList, TRUE);

  /* Set to be flushed bit */
  defer->list_bits |= (1 << list_index);

  status = STATUS_SUCCESS;

 end:
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static NTSTATUS
fort_defer_stream_add (PFORT_DEFER defer,
                       const FWPS_INCOMING_VALUES0 *inFixedValues,
                       const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                       const FWPS_STREAM_DATA0 *streamData,
                       const FWPS_FILTER0 *filter,
                       BOOL inbound)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_PACKET pkt;
  NTSTATUS status;

  if (defer->stream_injection4_id == INVALID_HANDLE_VALUE)
    return STATUS_FWP_TCPIP_NOT_READY;

  /* Check data offset compatibility */
  {
    const PNET_BUFFER netBuf = NET_BUFFER_LIST_FIRST_NB(streamData->netBufferListChain);
    const FWPS_STREAM_DATA_OFFSET0 *dataOffset = &streamData->dataOffset;

    if (dataOffset->streamDataOffset != 0
        && (dataOffset->netBuffer != netBuf
        || dataOffset->mdl != NET_BUFFER_CURRENT_MDL(netBuf)))
      return STATUS_INVALID_PARAMETER;
  }

  KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);

  pkt = fort_defer_packet_get(defer);
  if (pkt == NULL) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto end;
  }

  fort_defer_list_add(&defer->stream_list, pkt);

  pkt->inbound = inbound;
  pkt->is_stream = TRUE;
  pkt->dataOffset = (UINT32) streamData->dataOffset.streamDataOffset;
  pkt->dataSize = (UINT32) streamData->dataLength;
  pkt->netBufList = streamData->netBufferListChain;
  pkt->next = NULL;

  {
    PFORT_PACKET_STREAM pkt_stream = &pkt->stream;

    pkt_stream->layerId = inFixedValues->layerId;
    pkt_stream->streamFlags = streamData->flags;
    pkt_stream->calloutId = filter->action.calloutId;
    pkt_stream->flow_id = inMetaValues->flowHandle;
  }

  FwpsReferenceNetBufferList0(pkt->netBufList, TRUE);

  status = STATUS_SUCCESS;

 end:
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static void
fort_defer_packet_free (PFORT_DEFER defer,
                        PFORT_PACKET pkt,
                        PNET_BUFFER_LIST clonedNetBufList,
                        BOOL dispatchLevel)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  if (clonedNetBufList != NULL) {
    const NTSTATUS status = clonedNetBufList->Status;

    if (!NT_SUCCESS(status)) {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                 "FORT: Defer: Injection error: %x\n", status);
    }

    FwpsFreeCloneNetBufferList0(clonedNetBufList, 0);
  }

  FwpsDereferenceNetBufferList0(pkt->netBufList, FALSE);

  /* Add to free chain */
  if (dispatchLevel) {
    KeAcquireInStackQueuedSpinLockAtDpcLevel(&defer->lock, &lock_queue);
  } else {
    KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);
  }

  fort_defer_packet_put(defer, pkt);

  if (dispatchLevel) {
    KeReleaseInStackQueuedSpinLockFromDpcLevel(&lock_queue);
  } else {
    KeReleaseInStackQueuedSpinLock(&lock_queue);
  }
}

static void
fort_defer_packet_inject (PFORT_DEFER defer,
                          PFORT_PACKET pkt,
                          FORT_INJECT_COMPLETE_FUNC complete_func,
                          BOOL dispatchLevel)
{
  while (pkt != NULL) {
    PFORT_PACKET pkt_next = pkt->next;
    PNET_BUFFER_LIST clonedNetBufList = NULL;
    NTSTATUS status;

    FORT_INJECT_FUNC inject_func = pkt->is_stream
      ? &fort_packet_inject_stream
      : (pkt->inbound ? &fort_packet_inject_in : &fort_packet_inject_out);

    status = inject_func(defer, pkt, &clonedNetBufList, complete_func);

    if (!NT_SUCCESS(status)) {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                 "FORT: Defer: Injection prepare error: %x\n", status);

      if (clonedNetBufList != NULL) {
        clonedNetBufList->Status = STATUS_SUCCESS;
      }

      fort_defer_packet_free(defer, pkt, clonedNetBufList, dispatchLevel);
    }

    pkt = pkt_next;
  }
}

static void
fort_defer_packet_flush (PFORT_DEFER defer,
                         FORT_INJECT_COMPLETE_FUNC complete_func,
                         UINT32 list_bits,
                         BOOL dispatchLevel)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_DEFER_LIST defer_list;
  PFORT_PACKET pkt_chain;
  int i;

  list_bits &= defer->list_bits;
  if (list_bits == 0)
    return;

  pkt_chain = NULL;

  if (dispatchLevel) {
    KeAcquireInStackQueuedSpinLockAtDpcLevel(&defer->lock, &lock_queue);
  } else {
    KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);
  }

  for (i = 0; list_bits != 0; ++i) {
    const UINT32 list_bit = (list_bits & 1);

    list_bits >>= 1;
    if (list_bit == 0)
      continue;

    defer_list = &defer->lists[i];

    pkt_chain = fort_defer_list_get(defer_list, pkt_chain);
  }

  /* Clear flushed bits */
  defer->list_bits &= ~list_bits;

  if (dispatchLevel) {
    KeReleaseInStackQueuedSpinLockFromDpcLevel(&lock_queue);
  } else {
    KeReleaseInStackQueuedSpinLock(&lock_queue);
  }

  fort_defer_packet_inject(defer, pkt_chain, complete_func, dispatchLevel);
}

static void
fort_defer_stream_flush (PFORT_DEFER defer,
                         FORT_INJECT_COMPLETE_FUNC complete_func,
                         UINT64 flow_id,
                         BOOL dispatchLevel)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_PACKET pkt_chain;

  if (dispatchLevel) {
    KeAcquireInStackQueuedSpinLockAtDpcLevel(&defer->lock, &lock_queue);
  } else {
    KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);
  }

  pkt_chain = fort_defer_list_flow_get(&defer->stream_list, NULL, flow_id);

  if (dispatchLevel) {
    KeReleaseInStackQueuedSpinLockFromDpcLevel(&lock_queue);
  } else {
    KeReleaseInStackQueuedSpinLock(&lock_queue);
  }

  fort_defer_packet_inject(defer, pkt_chain, complete_func, dispatchLevel);
}
