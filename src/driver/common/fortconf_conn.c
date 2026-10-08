/* Fort Firewall Driver Configuration: Connection's Filtering */

#include "fortconf_conn.h"

#include "fortdef.h"

inline static BOOL fort_conf_conn_is_local_broadcast(PCFORT_CONF_META_CONN conn)
{
    if (conn->isIPv6) {
        return conn->remote_ip.v2 == 0x2FF;
    }

    return conn->remote_ip.v4 == 0xFFFFFFFF;
}

FORT_API BOOL fort_conf_conn_local_allowed(
        PFORT_CONF_META_CONN conn, const FORT_CONF_FLAGS conf_flags)
{
    conn->is_broadcast = (UINT16) fort_conf_conn_is_local_broadcast(conn);

    if (conf_flags.filter_locals)
        return FALSE;

    /* Loopback */
    if (conn->is_loopback) {
        return !conf_flags.block_traffic;
    }

    /* Broadcast */
    if (conn->is_broadcast) {
        return !conf_flags.block_lan_traffic;
    }

    return FALSE;
}

inline static BOOL fort_conf_conn_zone_filtered(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, const FORT_APP_DATA app_data)
{
    if (app_data.zones.accept_mask == 0 && app_data.zones.reject_mask == 0)
        return FALSE;

    FORT_CONF_ZONES_CONN_FILTERED_OPT opt = {
        .rule_zones = app_data.zones,
    };

    if (filter->funcs->zones_conn_filtered(filter->ctx, conn, &opt)) {
        if (opt.reject.included) {
            conn->act.zone_id = opt.reject.zone_id;
            conn->act.blocked = TRUE;
            return TRUE; /* block Rejected Zones */
        }

        if (opt.accept.filtered) {
            conn->act.zone_id = opt.accept.zone_id;
            conn->act.blocked = !opt.accept.included;
            return TRUE; /* allow/block-not Accepted Zones */
        }
    }

    return FALSE;
}

inline static BOOL fort_conf_conn_rule_id_filtered(
        PFORT_CONF_META_CONN conn, UINT16 rule_id, UCHAR reason)
{
    if (rule_id == 0)
        return FALSE;

    if (conn->rule_id == 0) {
        conn->rule_id = rule_id;
    }
    conn->reason = reason;

    return TRUE;
}

inline static BOOL fort_conf_conn_rule_filtered(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, UINT16 rule_id)
{
    if (rule_id == 0)
        return FALSE;

    if (!filter->funcs->rules_conn_filtered(filter->ctx, conn, rule_id))
        return FALSE;

    return fort_conf_conn_rule_id_filtered(conn, rule_id, FORT_CONN_REASON_RULE);
}

inline static BOOL fort_conf_conn_glob_rule_filtered(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, BOOL is_post, UCHAR reason)
{
    const UINT16 rule_id = filter->funcs->rules_glob_conn_filtered(filter->ctx, conn, is_post);

    return fort_conf_conn_rule_id_filtered(conn, rule_id, reason);
}

inline static BOOL fort_conf_conn_app_rule_filtered(PFORT_CONF_META_CONN conn)
{
    PCFORT_CONF_RULE rule = conn->app.rule;
    if (rule == NULL)
        return FALSE;

    if (!fort_conf_app_rule_conn_filtered(rule, conn))
        return FALSE;

    conn->reason = FORT_CONN_REASON_PROGRAM_FILTER;
    return TRUE;
}

inline static BOOL fort_conf_conn_app_direction_blocked(
        PFORT_CONF_META_CONN conn, const FORT_APP_FLAGS app_flags)
{
    if (conn->inbound) {
        if (!app_flags.block_inbound)
            return FALSE;

        conn->reason = FORT_CONN_REASON_BLOCK_INBOUND;
        return TRUE; /* block Inbound */
    }

    if (!app_flags.block_outbound)
        return FALSE;

    conn->reason = FORT_CONN_REASON_BLOCK_OUTBOUND;
    return TRUE; /* block Outbound */
}

inline static BOOL fort_conf_conn_app_flags_blocked(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, const FORT_APP_DATA app_data)
{
    if (app_data.flags.blocked) {
        conn->reason = FORT_CONN_REASON_PROGRAM;
        return TRUE; /* block Program */
    }

    if (fort_conf_conn_app_direction_blocked(conn, app_data.flags))
        return TRUE;

    if (app_data.flags.lan_only && !conn->is_local_net) {
        conn->reason = FORT_CONN_REASON_LAN_ONLY;
        return TRUE; /* block LAN Only */
    }

    if (filter->conf_flags.group_blocked
            && filter->funcs->groups_mask_blocked(filter->ctx, app_data.groups)) {
        conn->reason = FORT_CONN_REASON_GROUP;
        return TRUE; /* block Groups */
    }

    return FALSE;
}

inline static BOOL fort_conf_conn_groups_rule_filtered(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, UINT32 groups_mask)
{
    const UINT16 rule_id =
            filter->funcs->groups_rules_conn_filtered(filter->ctx, conn, groups_mask);

    return fort_conf_conn_rule_id_filtered(conn, rule_id, FORT_CONN_REASON_RULE);
}

static BOOL fort_conf_conn_app_filtered(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, const FORT_APP_DATA app_data)
{
    if (fort_conf_conn_app_flags_blocked(filter, conn, app_data)) {
        conn->act.blocked = TRUE;
        return TRUE; /* filtered by App Flags */
    }

    if (fort_conf_conn_zone_filtered(filter, conn, app_data)) {
        conn->reason = FORT_CONN_REASON_ZONE;
        return TRUE; /* filtered by Zones */
    }

    if (fort_conf_conn_groups_rule_filtered(filter, conn, app_data.groups))
        return TRUE; /* filtered by the Groups' Rules */

    if (fort_conf_conn_rule_filtered(filter, conn, app_data.rule_id))
        return TRUE; /* filtered by the Program's Rule */

    return fort_conf_conn_app_rule_filtered(conn);
}

inline static void fort_conf_conn_app_filter(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, const FORT_APP_DATA app_data)
{
    if (fort_conf_conn_glob_rule_filtered(
                filter, conn, /*is_post=*/FALSE, FORT_CONN_REASON_RULE_GLOB_PRE)) {
        return; /* filtered by Global Rule Pre Apps */
    }

    const BOOL app_found = (app_data.flags.found != 0);
    if (app_found ? fort_conf_conn_app_filtered(filter, conn, app_data) : conn->act.blocked) {
        return; /* filtered by App or Filter Mode */
    }

    if (fort_conf_conn_glob_rule_filtered(
                filter, conn, /*is_post=*/TRUE, FORT_CONN_REASON_RULE_GLOB_POST)) {
        return; /* filtered by Global Rule Post Apps */
    }

    if (app_found) {
        conn->act.blocked = FALSE; /* allow App */
        conn->reason = FORT_CONN_REASON_PROGRAM;
    }
}

inline static BOOL fort_conf_conn_filter_mode_filtered(
        PFORT_CONF_META_CONN conn, const FORT_CONF_FLAGS conf_flags)
{
    conn->reason = FORT_CONN_REASON_FILTER_MODE;

    /* Auto-Learn */
    if (conf_flags.allow_all_new) {
        conn->act.blocked = FALSE;
        return FALSE;
    }

    /* Ask to Connect */
    if (conf_flags.ask_to_connect) {
        conn->act.blocked = FALSE;
        conn->ask_to_connect = TRUE;
        return TRUE;
    }

    /* Block/Allow All */
    if (conf_flags.app_block_all || conf_flags.app_allow_all) {
        conn->act.blocked = (UCHAR) conf_flags.app_block_all;
        return FALSE;
    }

    /* Ignore */
    conn->act.blocked = TRUE;
    conn->ignore = TRUE;
    return TRUE;
}

FORT_API BOOL fort_conf_conn_app_allowed(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn, const FORT_APP_DATA app_data)
{
    if (!conn->act.blocked)
        return TRUE; /* collect traffic, when Filter Disabled */

    const BOOL app_found = (app_data.flags.found != 0);
    if (app_found || !fort_conf_conn_filter_mode_filtered(conn, filter->conf_flags)) {
        fort_conf_conn_app_filter(filter, conn, app_data);
    }

    return !conn->act.blocked;
}

inline static BOOL fort_conf_conn_lan_flags_filtered(
        PFORT_CONF_META_CONN conn, const FORT_CONF_FLAGS conf_flags)
{
    if (conf_flags.block_lan_traffic && !conn->is_loopback) {
        return TRUE; /* block LAN */
    }

    if (!conf_flags.filter_local_net) {
        conn->act.blocked = FALSE;
        return TRUE; /* allow Local Network */
    }

    return FALSE;
}

inline static BOOL fort_conf_conn_inet_flags_filtered(
        PFORT_CONF_META_CONN conn, const FORT_CONF_FLAGS conf_flags)
{
    if (conf_flags.block_inet_traffic && !conn->is_broadcast) {
        return TRUE; /* block Internet */
    }

    return FALSE;
}

inline static BOOL fort_conf_conn_net_flags_filtered(
        PFORT_CONF_META_CONN conn, const FORT_CONF_FLAGS conf_flags)
{
    if (conn->is_local_net) {
        return fort_conf_conn_lan_flags_filtered(conn, conf_flags);
    } else {
        return fort_conf_conn_inet_flags_filtered(conn, conf_flags);
    }
}

static BOOL fort_conf_conn_addr_group_included(PCFORT_CONF_CONN_FILTER filter,
        PCFORT_CONF_META_CONN conn, int addr_group_index, UCHAR *zone_id)
{
    const FORT_CONF_ADDR_GROUP_IP_INCLUDED_OPT opt = {
        .zone_func = filter->funcs->zones_ip_included,
        .ctx = filter->ctx,
        .addr_group_index = addr_group_index,
        .zone_id = zone_id,
    };

    return fort_conf_addr_group_ip_included(filter->conf, conn, &opt);
}

inline static BOOL fort_conf_conn_filter_flags_filtered(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn)
{
    const FORT_CONF_FLAGS conf_flags = filter->conf_flags;

    if (conf_flags.block_traffic) {
        return TRUE; /* block all */
    }

    /* LAN addresses */
    {
        UCHAR local_zone_id;
        conn->is_local_net = !fort_conf_conn_addr_group_included(
                filter, conn, /*addr_group_index=*/0, &local_zone_id);

        if (fort_conf_conn_net_flags_filtered(conn, conf_flags)) {
            return TRUE; /* block net */
        }
    }

    /* INET addresses */
    if (!fort_conf_conn_addr_group_included(
                filter, conn, /*addr_group_index=*/1, &conn->act.zone_id)) {
        conn->reason = FORT_CONN_REASON_IP_INET;
        return TRUE; /* block address */
    }

    return FALSE;
}

FORT_API BOOL fort_conf_conn_flags_filtered(
        PCFORT_CONF_CONN_FILTER filter, PFORT_CONF_META_CONN conn)
{
    conn->act.blocked = TRUE;
    conn->reason = FORT_CONN_REASON_UNKNOWN;

    const FORT_CONF_FLAGS conf_flags = filter->conf_flags;

    if (conf_flags.filter_enabled) {
        return fort_conf_conn_filter_flags_filtered(filter, conn);
    }

    conn->act.blocked = FALSE;

    if (!(conf_flags.log_stat && conf_flags.log_stat_no_filter))
        return TRUE; /* allow (Filter Disabled) */

    return FALSE;
}
