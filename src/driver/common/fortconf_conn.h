#ifndef FORTCONF_CONN_H
#define FORTCONF_CONN_H

#include "fortconf.h"

typedef BOOL fort_conf_zones_conn_filtered_func(
        void *ctx, PCFORT_CONF_META_CONN conn, PFORT_CONF_ZONES_CONN_FILTERED_OPT opt);

typedef BOOL fort_conf_rules_conn_filtered_func(
        void *ctx, PFORT_CONF_META_CONN conn, UINT16 rule_id);

typedef UINT16 fort_conf_rules_glob_conn_filtered_func(
        void *ctx, PFORT_CONF_META_CONN conn, BOOL is_post);

typedef BOOL fort_conf_groups_mask_blocked_func(void *ctx, UINT32 groups_mask);

typedef UINT16 fort_conf_groups_rules_conn_filtered_func(
        void *ctx, PFORT_CONF_META_CONN conn, UINT32 groups_mask);

/* Access to the Zones, Rules and Groups: the driver locks them on each call */
typedef struct fort_conf_conn_filter_funcs
{
    fort_conf_zones_ip_included_func *zones_ip_included;
    fort_conf_zones_conn_filtered_func *zones_conn_filtered;
    fort_conf_rules_conn_filtered_func *rules_conn_filtered;
    fort_conf_rules_glob_conn_filtered_func *rules_glob_conn_filtered;
    fort_conf_groups_mask_blocked_func *groups_mask_blocked;
    fort_conf_groups_rules_conn_filtered_func *groups_rules_conn_filtered;
} FORT_CONF_CONN_FILTER_FUNCS, *PFORT_CONF_CONN_FILTER_FUNCS;

typedef const FORT_CONF_CONN_FILTER_FUNCS *PCFORT_CONF_CONN_FILTER_FUNCS;

typedef struct fort_conf_conn_filter
{
    FORT_CONF_FLAGS conf_flags;

    PCFORT_CONF conf;

    PCFORT_CONF_CONN_FILTER_FUNCS funcs;
    void *ctx; /* the funcs' context */
} FORT_CONF_CONN_FILTER, *PFORT_CONF_CONN_FILTER;

typedef const FORT_CONF_CONN_FILTER *PCFORT_CONF_CONN_FILTER;

#if defined(__cplusplus)
extern "C" {
#endif

/* Returns TRUE, when the local (loopback or broadcast) connection is allowed without the Conf */
FORT_API BOOL fort_conf_conn_local_allowed(
        PFORT_CONF_META_CONN conn, const FORT_CONF_FLAGS conf_flags);

/* Returns TRUE, when the connection is decided by the Conf's flags and addresses */
FORT_API BOOL fort_conf_conn_flags_filtered(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn);

/* Returns TRUE, when the connection is allowed by the Filter Mode and the App's data.
 * Call it only after fort_conf_conn_flags_filtered() returned FALSE: it relies on the conn's
 * act.blocked and is_local_net. Then check the conn's ask_to_connect and ignore too. */
FORT_API BOOL fort_conf_conn_app_allowed(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, const FORT_APP_DATA app_data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCONF_CONN_H
