#ifndef FORTCNF_HOST_H
#define FORTCNF_HOST_H

#include "fortcnf.h"

/* The IP added to the Zones or to a host() filter at runtime */
typedef struct fort_host_ip
{
    UCHAR isIPv6;
    UCHAR isFilter; /* the host() filter's IP, else the Zones' one */

    union {
        UINT32 zones_mask;
        UINT32 host_id; /* the host() filter's id */
    };

    union {
        INT64 expire_time; /* by KeQueryInterruptTime() */
        struct fort_host_ip *purge_next; /* the next IP to remove, while purging */
    };

    ip_addr_t ip;

    tommy_hashdyn_node map_node;
} FORT_HOST_IP, *PFORT_HOST_IP;

typedef const FORT_HOST_IP *PCFORT_HOST_IP;

typedef struct fort_host_ips
{
    UINT32 volatile count;

    INT64 purge_time; /* the full table isn't purged before it */

    FORT_POOL_LIST pool_list;

    tommy_hashdyn ips_map;

    EX_SPIN_LOCK lock;
} FORT_HOST_IPS, *PFORT_HOST_IPS;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_host_ips_open(PFORT_HOST_IPS host_ips);

FORT_API void fort_host_ips_close(PFORT_HOST_IPS host_ips);

FORT_API NTSTATUS fort_host_ips_add(PFORT_HOST_IPS host_ips, PCFORT_CONF_HOST_IPS conf_ips);

/* Removes the IPs' Zones not in the mask */
FORT_API void fort_host_ips_zones_keep(PFORT_HOST_IPS host_ips, UINT32 zones_mask);

/* Removes the host() filters' IPs */
FORT_API void fort_host_ips_filters_clear(PFORT_HOST_IPS host_ips);

/* Returns the Zones of the connection's remote IP */
FORT_API UINT32 fort_host_ips_zones_mask(PFORT_HOST_IPS host_ips, PCFORT_CONF_META_CONN conn);

/* Is the connection's remote IP added to the host() filter */
FORT_API BOOL fort_host_ips_filter_included(
        PFORT_HOST_IPS host_ips, PCFORT_CONF_META_CONN conn, UINT32 host_id);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_HOST_H
