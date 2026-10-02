#ifndef FORTPKT_H
#define FORTPKT_H

#include "fortdrv.h"

#include "common/fortconf.h"
#include "fortcoutarg.h"

#define FORT_PACKET_POOL_TAG 'KwfF'

typedef struct fort_packet_in
{
    IF_INDEX interfaceIndex;
    IF_INDEX subInterfaceIndex;
} FORT_PACKET_IN, *PFORT_PACKET_IN;

typedef const FORT_PACKET_IN *PCFORT_PACKET_IN;

typedef struct fort_packet_out
{
    WSACMSGHDR *controlData;
    ULONG controlDataLength;

    SCOPE_ID remoteScopeId;
    UINT64 endpointHandle;

    ip_addr_t remoteAddr;
} FORT_PACKET_OUT, *PFORT_PACKET_OUT;

typedef const FORT_PACKET_OUT *PCFORT_PACKET_OUT;

#define FORT_PACKET_INBOUND         0x01
#define FORT_PACKET_IP6             0x02
#define FORT_PACKET_IPSEC_PROTECTED 0x08
#define FORT_PACKET_TYPE_FLOW       0x10
#define FORT_PACKET_TYPE_PENDING    0x20
#define FORT_PACKET_TYPE_MASK       0x30

typedef struct fort_packet_io
{
    UCHAR flags;

    /* Data for re-injection */
    COMPARTMENT_ID compartmentId;
    PNET_BUFFER_LIST netBufList;
    union {
        FORT_PACKET_IN in;
        FORT_PACKET_OUT out;
    };
} FORT_PACKET_IO, *PFORT_PACKET_IO;

typedef const FORT_PACKET_IO *PCFORT_PACKET_IO;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API BOOL fort_packet_injected_by_self(PCFORT_CALLOUT_ARG ca);

FORT_API BOOL fort_packet_is_ipsec_protected(PCFORT_CALLOUT_ARG ca);

FORT_API void fort_packet_free(PFORT_PACKET_IO pkt);

FORT_API NTSTATUS fort_packet_inject(PFORT_PACKET_IO pkt);

FORT_API NTSTATUS fort_packet_fill(PCFORT_CALLOUT_ARG ca, PFORT_PACKET_IO pkt, UCHAR pkt_flags);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPKT_H
