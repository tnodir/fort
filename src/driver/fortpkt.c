/* Fort Firewall ACK Packets Deferring & Re-injection */

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
  IF_INDEX interfaceIndex;
  IF_INDEX subInterfaceIndex;

  UINT16 ipHeaderSize;
  UINT16 transportHeaderSize;

  UINT16 nblOffset;
} FORT_PACKET_IN, *PFORT_PACKET_IN;

typedef struct fort_packet_out {
  UINT32 remoteAddr4;

  SCOPE_ID remoteScopeId;
  UINT64 endpointHandle;
} FORT_PACKET_OUT, *PFORT_PACKET_OUT;

typedef struct fort_packet {
  BOOL inbound;

  COMPARTMENT_ID compartmentId;

  PNET_BUFFER_LIST netBufList;

  struct fort_packet *next;

  union {
    FORT_PACKET_IN in;
    FORT_PACKET_OUT out;
  };
} FORT_PACKET, *PFORT_PACKET;

typedef struct fort_defer {
  HANDLE injection4_id;

  PFORT_PACKET packet_head;
  PFORT_PACKET packet_tail;
  PFORT_PACKET packet_free;

  tommy_arrayof packets;

  KSPIN_LOCK lock;
} FORT_DEFER, *PFORT_DEFER;

typedef void (*FORT_INJECT_COMPLETE_FUNC) (PFORT_PACKET, PNET_BUFFER_LIST, BOOLEAN);


static void
fort_defer_open (PFORT_DEFER defer)
{
  NTSTATUS status;

  status = FwpsInjectionHandleCreate0(AF_INET, FWPS_INJECTION_TYPE_TRANSPORT,
    &defer->injection4_id);

  if (!NT_SUCCESS(status)) {
    defer->injection4_id = INVALID_HANDLE_VALUE;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
               "FORT: Defer: Injection init error: %x\n", status);
  }

  tommy_arrayof_init(&defer->packets, sizeof(FORT_PACKET));

  KeInitializeSpinLock(&defer->lock);
}

static void
fort_defer_close (PFORT_DEFER defer)
{
  FwpsInjectionHandleDestroy0(defer->injection4_id);

  tommy_arrayof_done(&defer->packets);
}

static NTSTATUS
fort_defer_add (PFORT_DEFER defer,
                const FWPS_INCOMING_VALUES0 *inFixedValues,
                const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                PNET_BUFFER_LIST netBufList,
                BOOL inbound)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_PACKET pkt;
  NTSTATUS status;

  if (defer->injection4_id == INVALID_HANDLE_VALUE)
    return STATUS_FWP_TCPIP_NOT_READY;

  /* Skip self injected packet */
  {
    const FWPS_PACKET_INJECTION_STATE state = FwpsQueryPacketInjectionState0(
      defer->injection4_id, netBufList, NULL);

    if (state == FWPS_PACKET_INJECTED_BY_SELF
        || state == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
      return STATUS_CANT_TERMINATE_SELF;
  }

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

  KeAcquireInStackQueuedSpinLock(&defer->lock, &lock_queue);

  if (defer->packet_free != NULL) {
    pkt = defer->packet_free;
    defer->packet_free = pkt->next;
  } else {
    const tommy_count_t size = tommy_arrayof_size(&defer->packets);

    /* TODO: tommy_arrayof_grow(): check calloc()'s result for NULL */
    if (tommy_arrayof_grow(&defer->packets, size + 1), 0) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto end;
    }

    pkt = tommy_arrayof_ref(&defer->packets, size);
  }

  if (defer->packet_tail == NULL) {
    defer->packet_head = defer->packet_tail = pkt;
  } else {
    defer->packet_tail->next = pkt;
    defer->packet_tail = pkt;
  }

  pkt->inbound = inbound;
  pkt->compartmentId = inMetaValues->compartmentId;
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

    pkt_in->interfaceIndex = inFixedValues->incomingValue[interfaceField].value.uint32;
    pkt_in->subInterfaceIndex = inFixedValues->incomingValue[subInterfaceField].value.uint32;

    pkt_in->ipHeaderSize = (UINT16) inMetaValues->ipHeaderSize;
    pkt_in->transportHeaderSize = (UINT16) inMetaValues->transportHeaderSize;

    pkt_in->nblOffset = (UINT16) NET_BUFFER_DATA_OFFSET(
      NET_BUFFER_LIST_FIRST_NB(netBufList));
  } else {
    PFORT_PACKET_OUT pkt_out = &pkt->out;

    const int remoteAddrField = inbound
      ? FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS
      : FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS;

    /* host-order -> network-order conversion */
    pkt_out->remoteAddr4 = HTONL(
      inFixedValues->incomingValue[remoteAddrField].value.uint32);

    pkt_out->remoteScopeId = inMetaValues->remoteScopeId;
    pkt_out->endpointHandle = inMetaValues->transportEndpointHandle;
  }

  FwpsReferenceNetBufferList0(netBufList, TRUE);

  status = STATUS_SUCCESS;

 end:
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return status;
}

static void
fort_defer_free (PFORT_DEFER defer, PFORT_PACKET pkt,
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

  pkt->next = defer->packet_free;
  defer->packet_free = pkt;

  if (dispatchLevel) {
    KeReleaseInStackQueuedSpinLockFromDpcLevel(&lock_queue);
  } else {
    KeReleaseInStackQueuedSpinLock(&lock_queue);
  }
}

static NTSTATUS
fort_defer_inject_in (PFORT_DEFER defer, PFORT_PACKET pkt,
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
  if (pkt_in->nblOffset != NET_BUFFER_DATA_OFFSET(netBuffer)) {
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
      defer->injection4_id, NULL, NULL, 0,
      AF_INET, pkt->compartmentId,
      pkt_in->interfaceIndex, pkt_in->subInterfaceIndex,
      *clonedNetBufList, complete_func, pkt);
  }

  return status;
}

static NTSTATUS
fort_defer_inject_out (PFORT_DEFER defer, PFORT_PACKET pkt,
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
      defer->injection4_id, NULL,
      pkt_out->endpointHandle, 0,
      &send_args, AF_INET, pkt->compartmentId,
      *clonedNetBufList, complete_func, pkt);
  }

  return status;
}

static void
fort_defer_dpc_flush (PFORT_DEFER defer, FORT_INJECT_COMPLETE_FUNC complete_func)
{
  KLOCK_QUEUE_HANDLE lock_queue;
  PFORT_PACKET pkt;

  KeAcquireInStackQueuedSpinLockAtDpcLevel(&defer->lock, &lock_queue);

  pkt = defer->packet_head;
  if (pkt != NULL) {
    defer->packet_head = defer->packet_tail = NULL;
  }

  KeReleaseInStackQueuedSpinLockFromDpcLevel(&lock_queue);

  while (pkt != NULL) {
    PFORT_PACKET pkt_next = pkt->next;
    PNET_BUFFER_LIST clonedNetBufList = NULL;
    NTSTATUS status;

    status = pkt->inbound
      ? fort_defer_inject_in(defer, pkt, &clonedNetBufList, complete_func)
      : fort_defer_inject_out(defer, pkt, &clonedNetBufList, complete_func);

    if (!NT_SUCCESS(status)) {
      if (clonedNetBufList != NULL) {
        clonedNetBufList->Status = status;
      } else {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                   "FORT: Defer: Injection prepare error: %x\n", status);
      }

      fort_defer_free(defer, pkt, clonedNetBufList, TRUE);
    }

    pkt = pkt_next;
  }
}
