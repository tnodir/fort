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
    FWPS_CLASSIFY_OUT0 *classifyOut;
    UINT64 flowContext;

    UINT32 dataSize;

    UCHAR inbound : 1;
    UCHAR isIPv6 : 1;
} FORT_CALLOUT_ARG, *PFORT_CALLOUT_ARG;

typedef const FORT_CALLOUT_ARG *PCFORT_CALLOUT_ARG;

typedef struct fort_callout_ale_extra
{
    UCHAR app_data_found : 1;
    UCHAR drop_blocked : 1;
    UCHAR ignore : 1;
    UCHAR is_path_filled : 1;
    UCHAR is_conn_filled : 1;

    FORT_APP_DATA app_data;

    FORT_CONF_META_CONN conn;

    PIRP irp;
    ULONG_PTR info;
} FORT_CALLOUT_ALE_EXTRA, *PFORT_CALLOUT_ALE_EXTRA;

typedef const FORT_CALLOUT_ALE_EXTRA *PCFORT_CALLOUT_ALE_EXTRA;

#endif // FORTCOUTARG_H
