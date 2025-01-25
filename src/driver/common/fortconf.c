/* Fort Firewall Driver Configuration */

#include "fortconf.h"

#include <assert.h>

#include "fort_wildmatch.h"
#include "fortdef.h"

static_assert(sizeof(ip6_addr_t) == 16, "ip6_addr_t size mismatch");

static_assert(sizeof(FORT_CONF_FLAGS) == sizeof(UINT64), "FORT_CONF_FLAGS size mismatch");
static_assert(
        sizeof(FORT_CONF_RULE_FILTER) == sizeof(UINT32), "FORT_CONF_RULE_FILTER size mismatch");
static_assert(sizeof(FORT_CONF_RULE) == sizeof(UINT16), "FORT_CONF_RULE size mismatch");

static_assert((FORT_CONF_RULE_GLOBAL_MAX + FORT_CONF_RULE_SET_MAX) < 256,
        "FORT_CONF_RULE_GLOBAL_MAX count mismatch");

static_assert(sizeof(FORT_TRAF) == sizeof(UINT64), "FORT_TRAF size mismatch");
static_assert(sizeof(FORT_APP_FLAGS) == sizeof(UINT16), "FORT_APP_FLAGS size mismatch");
static_assert(sizeof(FORT_APP_DATA) == 2 * sizeof(UINT32), "FORT_APP_DATA size mismatch");

static int bit_scan_forward(ULONG mask)
{
    unsigned long index;
    return _BitScanForward(&index, mask) ? index : -1;
}

static BOOL fort_conf_proto_find(const UINT8 *proto_arr, UINT8 proto, UINT16 count, BOOL is_range)
{
    if (count == 0)
        return FALSE;

    int low = 0;
    int high = count - 1;

    do {
        const int mid = (low + high) / 2;
        const UINT8 mid_proto = proto_arr[mid];

        if (proto < mid_proto)
            high = mid - 1;
        else if (proto > mid_proto)
            low = mid + 1;
        else
            return TRUE;
    } while (low <= high);

    if (!is_range)
        return FALSE;

    return high >= 0 && proto >= proto_arr[high] && proto <= proto_arr[count + high];
}

static BOOL fort_conf_port_find(const UINT16 *port_arr, UINT16 port, UINT16 count, BOOL is_range)
{
    if (count == 0)
        return FALSE;

    int low = 0;
    int high = count - 1;

    do {
        const int mid = (low + high) / 2;
        const UINT16 mid_port = port_arr[mid];

        if (port < mid_port)
            high = mid - 1;
        else if (port > mid_port)
            low = mid + 1;
        else
            return TRUE;
    } while (low <= high);

    if (!is_range)
        return FALSE;

    return high >= 0 && port >= port_arr[high] && port <= port_arr[count + high];
}

static BOOL fort_conf_ip4_find(const UINT32 *iparr, UINT32 ip, UINT32 count, BOOL is_range)
{
    if (count == 0)
        return FALSE;

    int low = 0;
    int high = count - 1;

    do {
        const int mid = (low + high) / 2;
        const UINT32 mid_ip = iparr[mid];

        if (ip < mid_ip)
            high = mid - 1;
        else if (ip > mid_ip)
            low = mid + 1;
        else
            return TRUE;
    } while (low <= high);

    if (!is_range)
        return FALSE;

    return high >= 0 && ip >= iparr[high] && ip <= iparr[count + high];
}

static BOOL fort_conf_ip6_find(
        const ip6_addr_t *iparr, const ip6_addr_t *ip, UINT32 count, BOOL is_range)
{
    if (count == 0)
        return FALSE;

    int low = 0;
    int high = count - 1;

    do {
        const int mid = (low + high) / 2;
        const ip6_addr_t *mid_ip = &iparr[mid];

        const int res = fort_ip6_cmp(ip, mid_ip);
        if (res < 0)
            high = mid - 1;
        else if (res > 0)
            low = mid + 1;
        else
            return TRUE;
    } while (low <= high);

    if (!is_range)
        return FALSE;

    return high >= 0 && fort_ip6_cmp(ip, &iparr[high]) >= 0
            && fort_ip6_cmp(ip, &iparr[count + high]) <= 0;
}

static int fort_conf_blob_index(const char *arr, const char *p, UINT32 blob_len, UINT32 count)
{
    if (count == 0)
        return FALSE;

    int low = 0;
    int high = count - 1;

    do {
        const int mid = (low + high) / 2;
        const char *mid_p = &arr[mid * blob_len];

        const int res = fort_mem_cmp(p, mid_p, blob_len);
        if (res < 0)
            high = mid - 1;
        else if (res > 0)
            low = mid + 1;
        else
            return mid;
    } while (low <= high);

    return -1;
}

#define fort_conf_proto_inarr(proto_arr, proto, count)                                             \
    fort_conf_proto_find(proto_arr, proto, count, /*is_range=*/FALSE)

#define fort_conf_proto_inrange(proto_range, proto, count)                                         \
    fort_conf_proto_find(proto_range, proto, count, /*is_range=*/TRUE)

#define fort_conf_proto_list_arr_ref(proto_list) (proto_list)->proto

#define fort_conf_proto_list_pair_ref(proto_list) &(proto_list)->proto[(proto_list)->proto_n]

#define fort_conf_port_inarr(port_arr, port, count)                                                \
    fort_conf_port_find(port_arr, port, count, /*is_range=*/FALSE)

#define fort_conf_port_inrange(port_range, port, count)                                            \
    fort_conf_port_find(port_range, port, count, /*is_range=*/TRUE)

#define fort_conf_port_list_arr_ref(port_list) (port_list)->port

#define fort_conf_port_list_pair_ref(port_list) &(port_list)->port[(port_list)->port_n]

#define fort_conf_ip4_inarr(iparr, ip, count)                                                      \
    fort_conf_ip4_find(iparr, ip, count, /*is_range=*/FALSE)

#define fort_conf_ip4_inrange(iprange, ip, count)                                                  \
    fort_conf_ip4_find(iprange, ip, count, /*is_range=*/TRUE)

#define fort_conf_addr_list_ip4_ref(addr_list) (addr_list)->ip

#define fort_conf_addr_list_pair4_ref(addr_list) &(addr_list)->ip[(addr_list)->ip_n]

#define fort_conf_ip6_inarr(iparr, ip, count)                                                      \
    fort_conf_ip6_find(iparr, ip, count, /*is_range=*/FALSE)

#define fort_conf_ip6_inrange(iprange, ip, count)                                                  \
    fort_conf_ip6_find(iprange, ip, count, /*is_range=*/TRUE)

#define fort_conf_addr_list_ip6_ref(addr6_list) ((ip6_addr_t *) (addr6_list)->ip)

#define fort_conf_addr_list_pair6_ref(addr6_list)                                                  \
    (fort_conf_addr_list_ip6_ref(addr6_list) + (addr6_list)->ip_n)

FORT_API int fort_mem_cmp(const void *p1, const void *p2, UINT32 len)
{
    const size_t n = RtlCompareMemory(p1, p2, len);
    return (n == len) ? 0 : (((unsigned char *) p1)[n] - ((unsigned char *) p2)[n]);
}

FORT_API BOOL fort_mem_eql(const void *p1, const void *p2, UINT32 len)
{
    return RtlCompareMemory(p1, p2, len) == len;
}

static BOOL fort_conf_proto_inlist(const UINT8 proto, PCFORT_CONF_PROTO_LIST proto_list)
{
    return fort_conf_proto_inarr(
                   fort_conf_proto_list_arr_ref(proto_list), proto, proto_list->proto_n)
            || fort_conf_proto_inrange(
                    fort_conf_proto_list_pair_ref(proto_list), proto, proto_list->pair_n);
}

static BOOL fort_conf_port_inlist(const UINT16 port, PCFORT_CONF_PORT_LIST port_list)
{
    return fort_conf_port_inarr(fort_conf_port_list_arr_ref(port_list), port, port_list->port_n)
            || fort_conf_port_inrange(
                    fort_conf_port_list_pair_ref(port_list), port, port_list->pair_n);
}

FORT_API BOOL fort_conf_ip_inlist(const ip_addr_t ip, PCFORT_CONF_ADDR_LIST addr_list, BOOL isIPv6)
{
    if (isIPv6) {
        const ip6_addr_t *ip6 = &ip.v6;
        PCFORT_CONF_ADDR_LIST addr6_list = (PCFORT_CONF_ADDR_LIST) ((PCCH) addr_list
                + FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n));

        return fort_conf_ip6_inarr(fort_conf_addr_list_ip6_ref(addr6_list), ip6, addr6_list->ip_n)
                || fort_conf_ip6_inrange(
                        fort_conf_addr_list_pair6_ref(addr6_list), ip6, addr6_list->pair_n);
    } else {
        return fort_conf_ip4_inarr(fort_conf_addr_list_ip4_ref(addr_list), ip.v4, addr_list->ip_n)
                || fort_conf_ip4_inrange(
                        fort_conf_addr_list_pair4_ref(addr_list), ip.v4, addr_list->pair_n);
    }
}

FORT_API PCFORT_CONF_ADDR_GROUP fort_conf_addr_group_ref(PCFORT_CONF conf, int addr_group_index)
{
    const UINT32 *addr_group_offsets = (const UINT32 *) (conf->data + conf->addr_groups_off);
    const char *addr_group_data = (const char *) addr_group_offsets;

    return (PCFORT_CONF_ADDR_GROUP) (addr_group_data + addr_group_offsets[addr_group_index]);
}

static BOOL fort_conf_ip_included_check(PCFORT_CONF_ADDR_LIST addr_list,
        fort_conf_zones_ip_included_func zone_func, void *ctx, const ip_addr_t remote_ip,
        UINT32 zones_mask, BOOL list_is_empty, BOOL isIPv6)
{
    return (!list_is_empty && fort_conf_ip_inlist(remote_ip, addr_list, isIPv6))
            || (zone_func != NULL && zone_func(ctx, zones_mask, remote_ip, isIPv6));
}

FORT_API BOOL fort_conf_ip_included(PCFORT_CONF conf, fort_conf_zones_ip_included_func zone_func,
        void *ctx, const ip_addr_t remote_ip, BOOL isIPv6, int addr_group_index)
{
    PCFORT_CONF_ADDR_GROUP addr_group = fort_conf_addr_group_ref(conf, addr_group_index);

    const BOOL include_all = addr_group->include_all;
    const BOOL exclude_all = addr_group->exclude_all;

    /* Include All */
    const BOOL ip_excluded = exclude_all
            ? TRUE
            : fort_conf_ip_included_check(fort_conf_addr_group_exclude_list_ref(addr_group),
                      zone_func, ctx, remote_ip, addr_group->exclude_zones,
                      addr_group->exclude_is_empty, isIPv6);
    if (include_all)
        return !ip_excluded;

    /* Exclude All */
    const BOOL ip_included = /* include_all ? TRUE : */
            fort_conf_ip_included_check(fort_conf_addr_group_include_list_ref(addr_group),
                    zone_func, ctx, remote_ip, addr_group->include_zones,
                    addr_group->include_is_empty, isIPv6);
    if (exclude_all)
        return ip_included;

    /* Include or Exclude */
    return ip_included && !ip_excluded;
}

FORT_API BOOL fort_conf_zones_ip_included(
        PCFORT_CONF_ZONES zones, UINT32 zones_mask, const ip_addr_t ip, BOOL isIPv6)
{
    zones_mask &= (zones->mask & zones->enabled_mask);

    while (zones_mask != 0) {
        const int zone_index = bit_scan_forward(zones_mask);

        if (zone_index == -1)
            break; /* never, but to avoid static analizers warning */

        const UINT32 addr_off = zones->addr_off[zone_index];
        PCFORT_CONF_ADDR_LIST addr_list = (PCFORT_CONF_ADDR_LIST) &zones->data[addr_off];

        if (fort_conf_ip_inlist(ip, addr_list, isIPv6))
            return TRUE;

        zones_mask ^= (1u << zone_index);
    }

    return FALSE;
}

static BOOL fort_conf_zones_masks_conn_check(PCFORT_CONF_ZONES zones, PCFORT_CONF_META_CONN conn,
        UINT32 zones_mask, FORT_CONF_ZONES_CONN_FILTERED_RESULT *result)
{
    if (zones_mask == 0)
        return FALSE;

    result->filtered = TRUE;
    result->included =
            (UCHAR) fort_conf_zones_ip_included(zones, zones_mask, conn->remote_ip, conn->isIPv6);

    return TRUE;
}

FORT_API BOOL fort_conf_zones_conn_filtered(
        PCFORT_CONF_ZONES zones, PCFORT_CONF_META_CONN conn, PFORT_CONF_ZONES_CONN_FILTERED_OPT opt)
{
    const BOOL accept_filtered = fort_conf_zones_masks_conn_check(
            zones, conn, opt->rule_zones.accept_mask, &opt->accept);

    const BOOL reject_filtered = fort_conf_zones_masks_conn_check(
            zones, conn, opt->rule_zones.reject_mask, &opt->reject);

    return accept_filtered || reject_filtered;
}

FORT_API BOOL fort_conf_app_exe_equal(PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path)
{
    const UINT16 path_len = path->len;

    if (path_len != app_entry->path_len)
        return FALSE;

    return fort_mem_eql(path->buffer, app_entry->path, path_len);
}

static BOOL fort_conf_app_wild_equal(PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path)
{
    return wildmatch(app_entry->path, path->buffer) == WM_MATCH;
}

typedef BOOL fort_conf_app_equal_func(PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path);

static FORT_APP_DATA fort_conf_app_find_loop(PCFORT_CONF conf, PCFORT_APP_PATH path,
        UINT32 apps_off, UINT16 apps_n, fort_conf_app_equal_func *app_equal_func)
{
    const FORT_APP_DATA app_data = { 0 };

    if (apps_n == 0)
        return app_data;

    const char *app_entries = (const char *) (conf->data + apps_off);

    do {
        PCFORT_APP_ENTRY app_entry = (PCFORT_APP_ENTRY) app_entries;

        if (app_equal_func(app_entry, path))
            return app_entry->app_data;

        app_entries += FORT_CONF_APP_ENTRY_SIZE(app_entry->path_len);
    } while (--apps_n != 0);

    return app_data;
}

FORT_API FORT_APP_DATA fort_conf_app_exe_find(PCFORT_CONF conf, PVOID context, PCFORT_APP_PATH path)
{
    UNUSED(context);

    return fort_conf_app_find_loop(
            conf, path, conf->exe_apps_off, conf->exe_apps_n, fort_conf_app_exe_equal);
}

inline static FORT_APP_DATA fort_conf_app_wild_find(PCFORT_CONF conf, PCFORT_APP_PATH path)
{
    return fort_conf_app_find_loop(
            conf, path, conf->wild_apps_off, conf->wild_apps_n, fort_conf_app_wild_equal);
}

inline static int fort_conf_app_prefix_cmp(PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path)
{
    UINT16 path_len = path->len;

    if (path_len > app_entry->path_len) {
        path_len = app_entry->path_len;
    }

    return fort_mem_cmp(path->buffer, app_entry->path, path_len);
}

inline static FORT_APP_DATA fort_conf_app_prefix_find(PCFORT_CONF conf, PCFORT_APP_PATH path)
{
    const FORT_APP_DATA app_data = { 0 };

    const UINT16 count = conf->prefix_apps_n;
    if (count == 0)
        return app_data;

    const char *data = conf->data;
    const UINT32 *app_offsets = (const UINT32 *) (data + conf->prefix_apps_off);

    const char *app_entries = (const char *) (app_offsets + count + 1);
    int low = 0;
    int high = count - 1;

    do {
        const int mid = (low + high) / 2;
        const UINT32 app_off = app_offsets[mid];
        PCFORT_APP_ENTRY app_entry = (PCFORT_APP_ENTRY) (app_entries + app_off);

        const int res = fort_conf_app_prefix_cmp(app_entry, path);

        if (res < 0) {
            high = mid - 1;
        } else if (res > 0) {
            low = mid + 1;
        } else {
            return app_entry->app_data;
        }
    } while (low <= high);

    return app_data;
}

FORT_API FORT_APP_DATA fort_conf_app_find(PCFORT_CONF conf, PCFORT_APP_PATH path,
        fort_conf_app_exe_find_func *exe_find_func, PVOID exe_context)
{
    FORT_APP_DATA app_data;

    app_data = exe_find_func(conf, exe_context, path);
    if (app_data.found != 0)
        return app_data;

    app_data = fort_conf_app_wild_find(conf, path);
    if (app_data.found != 0)
        return app_data;

    app_data = fort_conf_app_prefix_find(conf, path);

    return app_data;
}

FORT_API BOOL fort_conf_app_group_blocked(const FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data)
{
    const UINT16 app_group_bit = (1 << app_data.flags.group_index);

    if ((app_group_bit & conf_flags.group_bits) != 0)
        return FALSE;

    return conf_flags.group_blocked;
}

inline static BOOL fort_conf_rules_rt_conn_filtered_zones(
        PCFORT_CONF_RULES_RT rules_rt, PFORT_CONF_META_CONN conn, PCFORT_CONF_RULE rule)
{
    if (!rule->has_zones)
        return FALSE;

    PCFORT_CONF_ZONES zones = rules_rt->zones;
    if (!zones)
        return FALSE;

    FORT_CONF_ZONES_CONN_FILTERED_OPT opt = {
        .rule_zones = *((PCFORT_CONF_RULE_ZONES) (rule + 1)),
    };

    if (fort_conf_zones_conn_filtered(zones, conn, &opt)) {
        const BOOL accepted = (!opt.accept.filtered || opt.accept.included);
        const BOOL rejected = (opt.reject.filtered && opt.reject.included);

        if (accepted && !rejected) {
            conn->blocked = rule->blocked;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL fort_conf_rule_filter_check(
        PCFORT_CONF_RULE_FILTER rule_filter, PCFORT_CONF_META_CONN conn);

static BOOL fort_conf_rule_filter_list_check(
        PCFORT_CONF_RULE_FILTER rule_filter, PCFORT_CONF_META_CONN conn)
{
    const BOOL isAnd = (rule_filter->type == FORT_RULE_FILTER_TYPE_LIST_AND);

    const char *end = (const char *) rule_filter + rule_filter->size;
    const char *data = (const char *) (rule_filter + 1);

    do {
        PCFORT_CONF_RULE_FILTER sub_filter = (PCFORT_CONF_RULE_FILTER) data;

        const BOOL filter_res = fort_conf_rule_filter_check(sub_filter, conn);

        if (isAnd ? !filter_res : filter_res) {
            return filter_res;
        }

        data += sub_filter->size;
    } while (data < end);

    return isAnd;
}

static fort_conf_rule_filter_check_address(PCFORT_CONF_META_CONN conn, const void *data)
{
    return fort_conf_ip_inlist(conn->remote_ip, data, conn->isIPv6);
}

static fort_conf_rule_filter_check_port(PCFORT_CONF_META_CONN conn, const void *data)
{
    return fort_conf_port_inlist(conn->remote_port, data);
}

static fort_conf_rule_filter_check_local_address(PCFORT_CONF_META_CONN conn, const void *data)
{
    return fort_conf_ip_inlist(conn->local_ip, data, conn->isIPv6);
}

static fort_conf_rule_filter_check_local_port(PCFORT_CONF_META_CONN conn, const void *data)
{
    return fort_conf_port_inlist(conn->local_port, data);
}

static fort_conf_rule_filter_check_protocol(PCFORT_CONF_META_CONN conn, const void *data)
{
    return fort_conf_proto_inlist(conn->ip_proto, data);
}

static fort_conf_rule_filter_check_direction(PCFORT_CONF_META_CONN conn, const void *data)
{
    const UINT16 flags = ((PCFORT_CONF_RULE_FILTER_FLAGS) data)->flags;

    const UINT16 conn_flags =
            (conn->inbound ? FORT_RULE_FILTER_DIRECTION_IN : FORT_RULE_FILTER_DIRECTION_OUT);

    return (flags & conn_flags) != 0;
}

static fort_conf_rule_filter_check_area(PCFORT_CONF_META_CONN conn, const void *data)
{
    const UINT16 flags = ((PCFORT_CONF_RULE_FILTER_FLAGS) data)->flags;

    const UINT16 conn_flags = (conn->is_loopback ? FORT_RULE_FILTER_AREA_LOCALHOST : 0)
            | ((conn->is_local_net) ? FORT_RULE_FILTER_AREA_LAN : 0)
            | (!(conn->is_loopback || conn->is_local_net) ? FORT_RULE_FILTER_AREA_INET : 0);

    return (flags & conn_flags) != 0;
}

static fort_conf_rule_filter_check_profile(PCFORT_CONF_META_CONN conn, const void *data)
{
    const UINT16 flags = ((PCFORT_CONF_RULE_FILTER_FLAGS) data)->flags;

    const UINT16 conn_flags = (1 << conn->profile_id);

    return (flags & conn_flags) != 0;
}

static fort_conf_rule_filter_check_port_protocol(
        PCFORT_CONF_META_CONN conn, const void *data, UCHAR proto)
{
    if (conn->ip_proto != proto)
        return FALSE;

    return fort_conf_rule_filter_check_port(conn, data);
}

static fort_conf_rule_filter_check_port_tcp(PCFORT_CONF_META_CONN conn, const void *data)
{
    return fort_conf_rule_filter_check_port_protocol(conn, data, IpProto_TCP);
}

static fort_conf_rule_filter_check_port_udp(PCFORT_CONF_META_CONN conn, const void *data)
{
    return fort_conf_rule_filter_check_port_protocol(conn, data, IpProto_UDP);
}

typedef BOOL (*FORT_CONF_RULE_FILTER_CHECK_FUNC)(PCFORT_CONF_META_CONN conn, const void *data);

static const FORT_CONF_RULE_FILTER_CHECK_FUNC fort_conf_rule_filter_check_funcList[] = {
    &fort_conf_rule_filter_check_address, // FORT_RULE_FILTER_TYPE_ADDRESS,
    &fort_conf_rule_filter_check_port, // FORT_RULE_FILTER_TYPE_PORT,
    &fort_conf_rule_filter_check_local_address, // FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS,
    &fort_conf_rule_filter_check_local_port, // FORT_RULE_FILTER_TYPE_LOCAL_PORT,
    &fort_conf_rule_filter_check_protocol, // FORT_RULE_FILTER_TYPE_PROTOCOL,
    &fort_conf_rule_filter_check_direction, // FORT_RULE_FILTER_TYPE_DIRECTION,
    &fort_conf_rule_filter_check_area, // FORT_RULE_FILTER_TYPE_AREA,
    &fort_conf_rule_filter_check_profile, // FORT_RULE_FILTER_TYPE_PROFILE,
    // Complex types
    &fort_conf_rule_filter_check_port_tcp, // FORT_RULE_FILTER_TYPE_PORT_TCP,
    &fort_conf_rule_filter_check_port_udp, // FORT_RULE_FILTER_TYPE_PORT_UDP,
};

static BOOL fort_conf_rule_filter_check(
        PCFORT_CONF_RULE_FILTER rule_filter, PCFORT_CONF_META_CONN conn)
{
    assert(rule_filter->size != 0);

    const int filter_type = rule_filter->type;

    if (filter_type == FORT_RULE_FILTER_TYPE_LIST_OR
            || filter_type == FORT_RULE_FILTER_TYPE_LIST_AND) {
        return fort_conf_rule_filter_list_check(rule_filter, conn);
    }

    if (filter_type < FORT_RULE_FILTER_TYPE_ADDRESS || filter_type > FORT_RULE_FILTER_TYPE_PORT_UDP)
        return FALSE;

    const FORT_CONF_RULE_FILTER_CHECK_FUNC func = fort_conf_rule_filter_check_funcList[filter_type];

    const void *data = (const void *) (rule_filter + 1);

    BOOL filter_res = func(conn, data);

    if (rule_filter->is_not) {
        filter_res = !filter_res;
    }

    return filter_res;
}

inline static BOOL fort_conf_rules_rt_conn_filtered_filters(
        PFORT_CONF_META_CONN conn, PCFORT_CONF_RULE rule)
{
    if (!rule->has_filters)
        return FALSE;

    PCFORT_CONF_RULE_FILTER rule_filter =
            (PCFORT_CONF_RULE_FILTER) ((PCCH) rule + FORT_CONF_RULE_SIZE(rule));

    if (fort_conf_rule_filter_check(rule_filter, conn)) {
        conn->blocked = rule->blocked;
        return TRUE;
    }

    return FALSE;
}

inline static BOOL fort_conf_rules_rt_conn_filtered_sets(
        PCFORT_CONF_RULES_RT rules_rt, PFORT_CONF_META_CONN conn, PCFORT_CONF_RULE rule)
{
    const int set_count = rule->set_count;
    if (set_count == 0)
        return FALSE;

    const UINT16 *rule_ids =
            (const UINT16 *) ((PCCH) rule + FORT_CONF_RULE_SET_INDEXES_OFFSET(rule));

    for (int i = 0; i < set_count; ++i) {
        const UINT16 rule_id = rule_ids[i];

        if (fort_conf_rules_rt_conn_filtered(rules_rt, conn, rule_id) || !conn->blocked) {
            return TRUE;
        }
    }

    return FALSE;
}

inline static BOOL fort_conf_rules_rt_conn_filtered_terminate(
        PFORT_CONF_META_CONN conn, PCFORT_CONF_RULE rule)
{
    /* Terminating Rule? */
    if (rule->terminate) {
        conn->blocked = rule->term_blocked;
        return TRUE;
    }

    return FALSE;
}

inline static BOOL fort_conf_rules_rt_conn_filtered_check(
        PCFORT_CONF_RULES_RT rules_rt, PFORT_CONF_META_CONN conn, PCFORT_CONF_RULE rule)
{
    const BOOL filter_res = fort_conf_rules_rt_conn_filtered_zones(rules_rt, conn, rule)
            || fort_conf_rules_rt_conn_filtered_filters(conn, rule);

    const BOOL is_exclusive = (rule->exclusive && !rule->blocked);
    if (is_exclusive ? !filter_res : filter_res) {
        return filter_res;
    }

    if (fort_conf_rules_rt_conn_filtered_sets(rules_rt, conn, rule))
        return TRUE;

    return fort_conf_rules_rt_conn_filtered_terminate(conn, rule);
}

FORT_API BOOL fort_conf_rules_rt_conn_filtered(
        PCFORT_CONF_RULES_RT rules_rt, PFORT_CONF_META_CONN conn, UINT16 rule_id)
{
    if (rule_id == 0)
        return FALSE;

    PCFORT_CONF_RULE rule = fort_conf_rules_rt_rule(rules_rt, rule_id);

    if (!rule->enabled)
        return FALSE;

    return fort_conf_rules_rt_conn_filtered_check(rules_rt, conn, rule);
}

FORT_API BOOL fort_conf_rules_conn_filtered(
        PCFORT_CONF_RULES rules, PCFORT_CONF_ZONES zones, PFORT_CONF_META_CONN conn, UINT16 rule_id)
{
    if (rule_id > rules->max_rule_id)
        return FALSE;

    const FORT_CONF_RULES_RT rules_rt = fort_conf_rules_rt_make(rules, zones);

    return fort_conf_rules_rt_conn_filtered(&rules_rt, conn, rule_id);
}

FORT_API FORT_CONF_RULES_RT fort_conf_rules_rt_make(
        PCFORT_CONF_RULES rules, PCFORT_CONF_ZONES zones)
{
    const FORT_CONF_RULES_RT rules_rt = {
        .rule_offsets = (PUINT32) rules->data - 1, /* exclude zero index */
        .rules_data = rules->data,
        .zones = zones,
    };
    return rules_rt;
}
