#ifndef FORTSTAT_H
#define FORTSTAT_H

#include "fortdrv.h"

#include "common/fortconf.h"
#include "forttds.h"

#define FORT_STATUS_FLOW_BLOCK STATUS_NOT_SAME_DEVICE

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_stat_proc
{
    struct fort_stat_proc *next;
    struct fort_stat_proc *prev;

    union {
#if defined(_WIN64)
        FORT_TRAF traf;
#else
        UINT32 process_id;
#endif
        void *data; /* tommy_hashdyn_node::data */
    };

    tommy_key_t proc_hash; /* tommy_hashdyn_node::index */

#if defined(_WIN64)
    UINT32 process_id;
#else
    FORT_TRAF traf;
#endif

    UINT16 proc_index;

    UINT16 log_stat : 1;
    UINT16 active : 1;

    UINT32 refcount;

    struct fort_stat_proc *next_active;
} FORT_STAT_PROC, *PFORT_STAT_PROC;

#define FORT_FLOW_SPEED_LIMIT_IN    0x01
#define FORT_FLOW_SPEED_LIMIT_OUT   0x02
#define FORT_FLOW_SPEED_LIMIT_PROC  0x04
#define FORT_FLOW_SPEED_LIMIT_FLAGS 0x07
#define FORT_FLOW_TCP               0x10
#define FORT_FLOW_IP6               0x20
#define FORT_FLOW_INBOUND           0x40
#define FORT_FLOW_ACTIVE            0x80

typedef struct fort_flow_opt
{
    union {
        struct
        {
            UCHAR volatile flags;
            UCHAR group_index;
            UINT16 proc_index;
        };

        UINT32 v;
    };
} FORT_FLOW_OPT, *PFORT_FLOW_OPT;

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_flow
{
    struct fort_flow *next;
    struct fort_flow *prev;

    union {
#if defined(_WIN64)
        UINT64 flow_id;
#else
        FORT_FLOW_OPT opt;
#endif
        void *data; /* tommy_hashdyn_node::data */
    };

    tommy_key_t flow_hash; /* tommy_hashdyn_node::index */

#if defined(_WIN64)
    FORT_FLOW_OPT opt;
#else
    UINT64 flow_id;
#endif
} FORT_FLOW, *PFORT_FLOW;

#define FORT_STAT_LOG                 0x01
#define FORT_STAT_SYSTEM_TIME_CHANGED 0x02
#define FORT_STAT_CLOSED              0x10 /* used on driver unloading */

#define FORT_STAT_ALE_CALLOUT_IDS_COUNT    4
#define FORT_STAT_PACKET_CALLOUT_IDS_COUNT 8
#define FORT_STAT_CALLOUT_IDS_COUNT                                                                \
    (FORT_STAT_ALE_CALLOUT_IDS_COUNT + FORT_STAT_PACKET_CALLOUT_IDS_COUNT)

#define FORT_STAT_ALE_CALLOUT_IDS_INDEX    0
#define FORT_STAT_PACKET_CALLOUT_IDS_INDEX FORT_STAT_ALE_CALLOUT_IDS_COUNT

enum FORT_STAT_CALLOUT_ID_TYPE {
    FORT_STAT_CONNECT4_ID = 0,
    FORT_STAT_CONNECT6_ID,
    FORT_STAT_ACCEPT4_ID,
    FORT_STAT_ACCEPT6_ID,
    FORT_STAT_STREAM4_ID,
    FORT_STAT_STREAM6_ID,
    FORT_STAT_DATAGRAM4_ID,
    FORT_STAT_DATAGRAM6_ID,
    FORT_STAT_IN_TRANSPORT4_ID,
    FORT_STAT_IN_TRANSPORT6_ID,
    FORT_STAT_OUT_TRANSPORT4_ID,
    FORT_STAT_OUT_TRANSPORT6_ID,
};

typedef struct fort_stat
{
    UCHAR volatile flags;

    UINT16 proc_active_count;

    LONG volatile flow_closing_count;

    UINT32 callout_ids[FORT_STAT_CALLOUT_IDS_COUNT];

    PFORT_STAT_PROC proc_free;
    PFORT_STAT_PROC proc_active;

    PFORT_FLOW flow_free;

    tommy_arrayof procs;
    tommy_hashdyn procs_map;

    tommy_arrayof flows;
    tommy_hashdyn flows_map;

    FORT_CONF_GROUP conf_group;

    LARGE_INTEGER system_time;

    KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API UCHAR fort_stat_flags_set(PFORT_STAT stat, UCHAR flags, BOOL on);

FORT_API UCHAR fort_stat_flags(PFORT_STAT stat);

FORT_API UCHAR fort_flow_flags_set(PFORT_FLOW flow, UCHAR flags, BOOL on);

FORT_API UCHAR fort_flow_flags(PFORT_FLOW flow);

FORT_API void fort_stat_open(PFORT_STAT stat);

FORT_API void fort_stat_close_flows(PFORT_STAT stat);

FORT_API void fort_stat_close(PFORT_STAT stat);

FORT_API void fort_stat_log_update(PFORT_STAT stat, BOOL log_stat);

FORT_API void fort_stat_conf_update(PFORT_STAT stat, const PFORT_CONF_IO conf_io);

FORT_API void fort_stat_conf_flags_update(PFORT_STAT stat, const PFORT_CONF_FLAGS conf_flags);

FORT_API NTSTATUS fort_flow_associate(PFORT_STAT stat, UINT64 flow_id, UINT32 process_id,
        UCHAR group_index, BOOL isIPv6, BOOL is_tcp, BOOL inbound, BOOL is_reauth, BOOL *log_stat);

FORT_API void fort_flow_delete(PFORT_STAT stat, UINT64 flowContext);

FORT_API void fort_flow_classify(
        PFORT_STAT stat, UINT64 flowContext, UINT32 data_len, BOOL inbound);

FORT_API void fort_stat_dpc_begin(PFORT_STAT stat, PKLOCK_QUEUE_HANDLE lock_queue);

FORT_API void fort_stat_dpc_end(PKLOCK_QUEUE_HANDLE lock_queue);

FORT_API void fort_stat_traf_flush(PFORT_STAT stat, UINT16 proc_count, PCHAR out);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTSTAT_H
