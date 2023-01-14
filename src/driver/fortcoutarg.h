#ifndef FORTCOUTARG_H
#define FORTCOUTARG_H

#include "fortdrv.h"

typedef struct fort_callout_arg
{
    const FWPS_INCOMING_VALUES0 *inFixedValues;
    const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues;
    union {
        const PNET_BUFFER_LIST netBufList;
        const FWPS_STREAM_CALLOUT_IO_PACKET0 *packet;
    };
    const FWPS_FILTER0 *filter;
    UINT64 flowContext;
    FWPS_CLASSIFY_OUT0 *classifyOut;

    BOOL inbound : 1;
    BOOL isIPv6 : 1;
} FORT_CALLOUT_ARG, *PFORT_CALLOUT_ARG;

typedef struct fort_callout_ale_index
{
    UCHAR flags;
    UCHAR localIp;
    UCHAR remoteIp;
    UCHAR localPort;
    UCHAR remotePort;
    UCHAR ipProto;
} FORT_CALLOUT_ALE_INDEX, *PFORT_CALLOUT_ALE_INDEX;

typedef struct fort_callout_datagram_index
{
    UCHAR direction;
} FORT_CALLOUT_DATAGRAM_INDEX, *PFORT_CALLOUT_DATAGRAM_INDEX;

#endif // FORTCOUTARG_H
