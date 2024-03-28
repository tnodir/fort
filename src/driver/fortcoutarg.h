#ifndef FORTCOUTARG_H
#define FORTCOUTARG_H

#include "fortdrv.h"

#include "common/fortconf.h"

typedef struct fort_callout_field_index
{
    UCHAR flags;
    UCHAR localIp;
    UCHAR remoteIp;
    UCHAR localPort;
    UCHAR remotePort;
    UCHAR ipProto;
    UCHAR direction; /* used by DATAGRAM only */
} FORT_CALLOUT_FIELD_INDEX, *PFORT_CALLOUT_FIELD_INDEX;

typedef const FORT_CALLOUT_FIELD_INDEX *PCFORT_CALLOUT_FIELD_INDEX;

typedef struct fort_callout_arg
{
    PCFORT_CALLOUT_FIELD_INDEX fi;

    const FWPS_INCOMING_VALUES0 *inFixedValues;
    const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues;
    union {
        const PNET_BUFFER_LIST netBufList;
        const FWPS_STREAM_CALLOUT_IO_PACKET0 *packet;
    };
    const FWPS_FILTER0 *filter;
    UINT64 flowContext;
    FWPS_CLASSIFY_OUT0 *classifyOut;

    UCHAR inbound : 1;
    UCHAR isIPv6 : 1;
} FORT_CALLOUT_ARG, *PFORT_CALLOUT_ARG;

typedef const FORT_CALLOUT_ARG *PCFORT_CALLOUT_ARG;

typedef struct fort_callout_ale_extra
{
    UCHAR is_reauth : 1;
    UCHAR app_data_found : 1;
    UCHAR inherited : 1;
    UCHAR drop_blocked : 1;
    UCHAR blocked : 1;
    UCHAR ignore : 1;
    INT8 block_reason;

    FORT_APP_DATA app_data;

    UINT32 process_id;

    const UINT32 *remote_ip;

    PCUNICODE_STRING path;
    PCUNICODE_STRING real_path;

    PIRP irp;
    ULONG_PTR info;
} FORT_CALLOUT_ALE_EXTRA, *PFORT_CALLOUT_ALE_EXTRA;

typedef const FORT_CALLOUT_ALE_EXTRA *PCFORT_CALLOUT_ALE_EXTRA;

#endif // FORTCOUTARG_H
