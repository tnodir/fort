/* Fort Firewall Driver Configuration */

#include "fortconf.h"

#include <assert.h>

#include "fort_wildmatch.h"
#include "fortdef.h"

static_assert(sizeof(ip6_addr_t) == 16, "ip6_addr_t size mismatch");

static_assert(sizeof(FORT_CONF_FLAGS) == sizeof(UINT32), "FORT_CONF_FLAGS size mismatch");
static_assert(sizeof(FORT_CONF_RULE_EXPR) == sizeof(UINT16), "FORT_CONF_RULE_EXPR size mismatch");
static_assert(sizeof(FORT_CONF_RULE) == sizeof(UINT16), "FORT_CONF_RULE size mismatch");
static_assert(sizeof(FORT_TRAF) == sizeof(UINT64), "FORT_TRAF size mismatch");
static_assert(sizeof(FORT_TIME) == sizeof(UINT16), "FORT_TIME size mismatch");
static_assert(sizeof(FORT_PERIOD) == sizeof(UINT32), "FORT_PERIOD size mismatch");
static_assert(sizeof(FORT_APP_FLAGS) == sizeof(UINT16), "FORT_APP_FLAGS size mismatch");
static_assert(sizeof(FORT_APP_DATA) == 2 * sizeof(UINT32), "FORT_APP_DATA size mismatch");

#ifndef FORT_DRIVER
#    define fort_memcmp memcmp
#else
static int fort_memcmp(const void *p1, const void *p2, size_t len)
{
    const size_t n = RtlCompareMemory(p1, p2, len);
    return (n == len) ? 0 : (((const char *) p1)[n] - ((const char *) p2)[n]);
}
#endif

FORT_API BOOL is_time_in_period(FORT_TIME time, FORT_PERIOD period)
{
    const int x = time.hour * 60 + time.minute;
    const int from = period.from.hour * 60 + period.from.minute;
    const int to = period.to.hour * 60 + period.to.minute;

    return (from <= to ? (x >= from && x < (to - 1)) : (x >= from || x < (to - 1)));
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

#define fort_ip6_cmp(l, r) fort_memcmp(l, r, sizeof(ip6_addr_t))

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

#define fort_conf_addr_list_ip6_ref(addr6_list) (addr6_list)->ip

#define fort_conf_addr_list_pair6_ref(addr6_list) &(addr6_list)->ip[(addr6_list)->ip_n]

FORT_API BOOL fort_conf_ip_inlist(
        const UINT32 *ip, const PFORT_CONF_ADDR4_LIST addr_list, BOOL isIPv6)
{
    if (isIPv6) {
        const ip6_addr_t *ip6 = (const ip6_addr_t *) ip;
        const PFORT_CONF_ADDR6_LIST addr6_list =
                (const PFORT_CONF_ADDR6_LIST)((const PCHAR) addr_list
                        + FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n));

        return fort_conf_ip6_inarr(fort_conf_addr_list_ip6_ref(addr6_list), ip6, addr6_list->ip_n)
                || fort_conf_ip6_inrange(
                        fort_conf_addr_list_pair6_ref(addr6_list), ip6, addr6_list->pair_n);
    } else {
        return fort_conf_ip4_inarr(fort_conf_addr_list_ip4_ref(addr_list), *ip, addr_list->ip_n)
                || fort_conf_ip4_inrange(
                        fort_conf_addr_list_pair4_ref(addr_list), *ip, addr_list->pair_n);
    }
}

FORT_API PFORT_CONF_ADDR_GROUP fort_conf_addr_group_ref(const PFORT_CONF conf, int addr_group_index)
{
    const UINT32 *addr_group_offsets = (const UINT32 *) (conf->data + conf->addr_groups_off);
    const char *addr_group_data = (const char *) addr_group_offsets;

    return (PFORT_CONF_ADDR_GROUP) (addr_group_data + addr_group_offsets[addr_group_index]);
}

static BOOL fort_conf_ip_included_check(const PFORT_CONF_ADDR4_LIST addr_list,
        fort_conf_zones_ip_included_func zone_func, void *ctx, const UINT32 *remote_ip,
        UINT32 zones_mask, BOOL list_is_empty, BOOL isIPv6)
{
    return (!list_is_empty && fort_conf_ip_inlist(remote_ip, addr_list, isIPv6))
            || (zone_func != NULL && zone_func(ctx, zones_mask, remote_ip, isIPv6));
}

FORT_API BOOL fort_conf_ip_included(const PFORT_CONF conf,
        fort_conf_zones_ip_included_func zone_func, void *ctx, const UINT32 *remote_ip, BOOL isIPv6,
        int addr_group_index)
{
    const PFORT_CONF_ADDR_GROUP addr_group = fort_conf_addr_group_ref(conf, addr_group_index);

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

FORT_API BOOL fort_conf_app_exe_equal(
        const PFORT_APP_ENTRY app_entry, const PVOID path, UINT32 path_len)
{
    if (path_len != app_entry->path_len)
        return FALSE;

    return fort_memcmp(path, app_entry->path, path_len) == 0;
}

static BOOL fort_conf_app_wild_equal(
        const PFORT_APP_ENTRY app_entry, const PVOID path, UINT32 path_len)
{
    UNUSED(path_len);

    return wildmatch(app_entry->path, (const WCHAR *) path) == WM_MATCH;
}

typedef BOOL fort_conf_app_equal_func(
        const PFORT_APP_ENTRY app_entry, const PVOID path, UINT32 path_len);

static FORT_APP_DATA fort_conf_app_find_loop(const PFORT_CONF conf, const PVOID path,
        UINT32 path_len, UINT32 apps_off, UINT16 apps_n, fort_conf_app_equal_func *app_equal_func)
{
    const FORT_APP_DATA app_data = { 0 };

    if (apps_n == 0)
        return app_data;

    const char *app_entries = (const char *) (conf->data + apps_off);

    do {
        const PFORT_APP_ENTRY app_entry = (const PFORT_APP_ENTRY) app_entries;

        if (app_equal_func(app_entry, path, path_len))
            return app_entry->app_data;

        app_entries += FORT_CONF_APP_ENTRY_SIZE(app_entry->path_len);
    } while (--apps_n != 0);

    return app_data;
}

FORT_API FORT_APP_DATA fort_conf_app_exe_find(
        const PFORT_CONF conf, PVOID context, const PVOID path, UINT32 path_len)
{
    UNUSED(context);

    return fort_conf_app_find_loop(
            conf, path, path_len, conf->exe_apps_off, conf->exe_apps_n, fort_conf_app_exe_equal);
}

static FORT_APP_DATA fort_conf_app_wild_find(
        const PFORT_CONF conf, const PVOID path, UINT32 path_len)
{
    return fort_conf_app_find_loop(
            conf, path, path_len, conf->wild_apps_off, conf->wild_apps_n, fort_conf_app_wild_equal);
}

static int fort_conf_app_prefix_cmp(PFORT_APP_ENTRY app_entry, const PVOID path, UINT32 path_len)
{
    if (path_len > app_entry->path_len) {
        path_len = app_entry->path_len;
    }

    return fort_memcmp(path, app_entry->path, path_len);
}

static FORT_APP_DATA fort_conf_app_prefix_find(
        const PFORT_CONF conf, const PVOID path, UINT32 path_len)
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
        const PFORT_APP_ENTRY app_entry = (PFORT_APP_ENTRY) (app_entries + app_off);

        const int res = fort_conf_app_prefix_cmp(app_entry, path, path_len);

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

FORT_API FORT_APP_DATA fort_conf_app_find(const PFORT_CONF conf, const PVOID path, UINT32 path_len,
        fort_conf_app_exe_find_func *exe_find_func, PVOID exe_context)
{
    FORT_APP_DATA app_data;

    app_data = exe_find_func(conf, exe_context, path, path_len);
    if (app_data.flags.v != 0)
        return app_data;

    app_data = fort_conf_app_wild_find(conf, path, path_len);
    if (app_data.flags.v != 0)
        return app_data;

    app_data = fort_conf_app_prefix_find(conf, path, path_len);

    return app_data;
}

static BOOL fort_conf_app_blocked_check(const PFORT_CONF conf, INT8 *block_reason, BOOL app_found,
        BOOL app_allowed, BOOL app_blocked)
{
    *block_reason = app_found ? FORT_BLOCK_REASON_APP_GROUP_FOUND : FORT_BLOCK_REASON_FILTER_MODE;

    /* Block All */
    if (conf->flags.app_block_all)
        return !app_allowed; /* Block, if it is not explicitly allowed */

    /* Allow All */
    if (conf->flags.app_allow_all)
        return app_blocked; /* Block, if it is explicitly blocked */

    /* Ignore */
    if (!app_found) {
        *block_reason = FORT_BLOCK_REASON_NONE; /* Don't block or allow */
        return TRUE;
    }

    /* Block, if it is explicitly blocked and not allowed */
    return app_blocked && !app_allowed;
}

FORT_API BOOL fort_conf_app_blocked(
        const PFORT_CONF conf, FORT_APP_FLAGS app_flags, INT8 *block_reason)
{
    const BOOL app_found = (app_flags.v != 0);

    if (app_found) {
        if (!app_flags.use_group_perm) {
            *block_reason = FORT_BLOCK_REASON_PROGRAM;
            return app_flags.blocked;
        }
    }

    const UINT32 app_perm_val = app_flags.blocked ? 2 : 1;
    const UINT32 app_perm = app_perm_val << (app_flags.group_index * 2);

    const BOOL app_allowed = app_found && (app_perm & conf->app_perms_allow_mask) != 0;
    const BOOL app_blocked = app_found && (app_perm & conf->app_perms_block_mask) != 0;

    return fort_conf_app_blocked_check(conf, block_reason, app_found, app_allowed, app_blocked);
}

FORT_API UINT16 fort_conf_app_period_bits(const PFORT_CONF conf, FORT_TIME time, int *periods_n)
{
    UINT8 count = conf->app_periods_n;

    if (count == 0)
        return 0;

    const char *data = conf->data;
    PFORT_PERIOD app_periods = (const PFORT_PERIOD)(data + conf->app_periods_off);
    UINT16 period_bits = (UINT16) conf->flags.group_bits;
    int n = 0;

    for (int i = 0; i < FORT_CONF_GROUP_MAX; ++i) {
        const UINT16 bit = (1 << i);
        const FORT_PERIOD period = *app_periods++;

        if ((period_bits & bit) != 0 && period.v != 0) {
            if (!is_time_in_period(time, period)) {
                period_bits ^= bit;
            }

            ++n;

            if (--count == 0)
                break;
        }
    }

    if (periods_n != NULL) {
        *periods_n = n;
    }

    return period_bits;
}

FORT_API void fort_conf_app_perms_mask_init(PFORT_CONF conf, UINT32 group_bits)
{
    UINT32 perms_mask = (group_bits & 0x0001) | ((group_bits & 0x0002) << 1)
            | ((group_bits & 0x0004) << 2) | ((group_bits & 0x0008) << 3)
            | ((group_bits & 0x0010) << 4) | ((group_bits & 0x0020) << 5)
            | ((group_bits & 0x0040) << 6) | ((group_bits & 0x0080) << 7)
            | ((group_bits & 0x0100) << 8) | ((group_bits & 0x0200) << 9)
            | ((group_bits & 0x0400) << 10) | ((group_bits & 0x0800) << 11)
            | ((group_bits & 0x1000) << 12) | ((group_bits & 0x2000) << 13)
            | ((group_bits & 0x4000) << 14) | ((group_bits & 0x8000) << 15);

    perms_mask |= perms_mask << 1;

    conf->app_perms_block_mask = (perms_mask & 0xAAAAAAAA);
    conf->app_perms_allow_mask = (perms_mask & 0x55555555);
}
