#ifndef FORTSTAT_H
#define FORTSTAT_H

#include "fortdrv.h"

#include "common/fortconf.h"
#include "forttds.h"

#define FORT_STATUS_FLOW_BLOCK STATUS_NOT_SAME_DEVICE

typedef struct fort_stat_group
{
    FORT_TRAF traf;
} FORT_STAT_GROUP, *PFORT_STAT_GROUP;

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

    UINT16 proc_index : 15; /* Synchronize with FORT_PROC_COUNT_MAX! */
    UINT16 active : 1;

    UINT16 refcount;

#if defined(_WIN64)
    UINT32 process_id;
#else
    FORT_TRAF traf;
#endif

    struct fort_stat_proc *next_active;
} FORT_STAT_PROC, *PFORT_STAT_PROC;

#define FORT_FLOW_SPEED_LIMIT_IN  0x01
#define FORT_FLOW_SPEED_LIMIT_OUT 0x02
#define FORT_FLOW_SPEED_LIMIT     (FORT_FLOW_SPEED_LIMIT_IN | FORT_FLOW_SPEED_LIMIT_OUT)
#define FORT_FLOW_DEFER_IN        0x04
#define FORT_FLOW_DEFER_OUT       0x08
#define FORT_FLOW_FRAGMENT        0x10
#define FORT_FLOW_FRAGMENT_DEFER  0x20
#define FORT_FLOW_FRAGMENTED      0x40
#define FORT_FLOW_XFLAGS          (FORT_FLOW_FRAGMENT_DEFER | FORT_FLOW_FRAGMENTED)

typedef struct fort_flow_opt
{
    UCHAR volatile flags;
    UCHAR group_index;
    UINT16 proc_index;
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

typedef struct fort_stat
{
    UCHAR volatile closed;

    UCHAR log_stat : 1;

    UINT16 proc_active_count;

    UINT32 group_flush_bits;

    UINT32 stream4_id;
    UINT32 datagram4_id;
    UINT32 in_transport4_id;
    UINT32 out_transport4_id;

    PFORT_STAT_PROC proc_free;
    PFORT_STAT_PROC proc_active;

    PFORT_FLOW flow_free;

    tommy_arrayof procs;
    tommy_hashdyn procs_map;

    tommy_arrayof flows;
    tommy_hashdyn flows_map;

    FORT_CONF_GROUP conf_group;

    FORT_STAT_GROUP groups[FORT_CONF_GROUP_MAX];

    LARGE_INTEGER system_time;

    KSPIN_LOCK lock;
} FORT_STAT, *PFORT_STAT;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API UCHAR fort_flow_flags_set(PFORT_FLOW flow, UCHAR flags, BOOL on);

FORT_API UCHAR fort_flow_flags(PFORT_FLOW flow);

FORT_API void fort_stat_open(PFORT_STAT stat);

FORT_API void fort_stat_close(PFORT_STAT stat);

FORT_API void fort_stat_update(PFORT_STAT stat, BOOL log_stat);

FORT_API void fort_stat_conf_update(PFORT_STAT stat, PFORT_CONF_IO conf_io);

FORT_API NTSTATUS fort_flow_associate(PFORT_STAT stat, UINT64 flow_id, UINT32 process_id,
        UCHAR group_index, BOOL is_tcp, BOOL is_reauth, BOOL *is_new_proc);

FORT_API void fort_flow_delete(PFORT_STAT stat, UINT64 flowContext);

FORT_API void fort_flow_classify(
        PFORT_STAT stat, UINT64 flowContext, UINT32 data_len, BOOL is_tcp, BOOL inbound);

FORT_API void fort_stat_dpc_begin(PFORT_STAT stat, PKLOCK_QUEUE_HANDLE lock_queue);

FORT_API void fort_stat_dpc_end(PKLOCK_QUEUE_HANDLE lock_queue);

FORT_API void fort_stat_dpc_traf_flush(PFORT_STAT stat, UINT16 proc_count, PCHAR out);

FORT_API UINT32 fort_stat_dpc_group_flush(PFORT_STAT stat);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTSTAT_H
