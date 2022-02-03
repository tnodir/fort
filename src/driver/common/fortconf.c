/* Fort Firewall Driver Configuration */

#include "fortconf.h"

#include "fortdef.h"
#include "wildmatch.h"

#ifndef FORT_DRIVER
#    define fort_memcmp memcmp
#else
static int fort_memcmp(const char *p1, const char *p2, size_t len)
{
    const size_t n = RtlCompareMemory(p1, p2, len);
    return (n == len) ? 0 : (p1[n] - p2[n]);
}
#endif

FORT_API int bit_scan_forward(unsigned long mask)
{
    unsigned long index;
    return _BitScanForward(&index, mask) ? index : -1;
}

FORT_API BOOL is_time_in_period(FORT_TIME time, FORT_PERIOD period)
{
    const int x = time.hour * 60 + time.minute;
    const int from = period.from.hour * 60 + period.from.minute;
    const int to = period.to.hour * 60 + period.to.minute;

    return (from <= to ? (x >= from && x < (to - 1)) : (x >= from || x < (to - 1)));
}

static BOOL fort_conf_ip_find(UINT32 ip, UINT32 count, const UINT32 *iparr, BOOL is_range)
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

static BOOL fort_conf_ip_inarr(UINT32 ip, UINT32 count, const UINT32 *iparr)
{
    return fort_conf_ip_find(ip, count, iparr, FALSE);
}

static BOOL fort_conf_ip_inrange(UINT32 ip, UINT32 count, const UINT32 *iprange)
{
    return fort_conf_ip_find(ip, count, iprange, TRUE);
}

#define fort_conf_addr_list_ip_ref(addr_list)   (addr_list)->ip
#define fort_conf_addr_list_pair_ref(addr_list) &(addr_list)->ip[(addr_list)->ip_n]

FORT_API BOOL fort_conf_ip_inlist(UINT32 ip, const PFORT_CONF_ADDR_LIST addr_list)
{
    return fort_conf_ip_inarr(ip, addr_list->ip_n, fort_conf_addr_list_ip_ref(addr_list))
            || fort_conf_ip_inrange(ip, addr_list->pair_n, fort_conf_addr_list_pair_ref(addr_list));
}

FORT_API PFORT_CONF_ADDR_GROUP fort_conf_addr_group_ref(const PFORT_CONF conf, int addr_group_index)
{
    const UINT32 *addr_group_offsets = (const UINT32 *) (conf->data + conf->addr_groups_off);
    const char *addr_group_data = (const char *) addr_group_offsets;

    return (PFORT_CONF_ADDR_GROUP) (addr_group_data + addr_group_offsets[addr_group_index]);
}

static BOOL fort_conf_ip_included_check(const PFORT_CONF_ADDR_LIST addr_list,
        fort_conf_zones_ip_included_func zone_func, void *ctx, UINT32 remote_ip, UINT32 zones_mask,
        BOOL list_is_empty)
{
    return (!list_is_empty && fort_conf_ip_inlist(remote_ip, addr_list))
            || (zone_func != NULL && zone_func(ctx, zones_mask, remote_ip));
}

FORT_API BOOL fort_conf_ip_included(const PFORT_CONF conf,
        fort_conf_zones_ip_included_func zone_func, void *ctx, UINT32 remote_ip,
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
                    addr_group->exclude_is_empty);
    if (include_all)
        return !ip_excluded;

    /* Exclude All */
    const BOOL ip_included = include_all
            ? TRUE
            : fort_conf_ip_included_check(fort_conf_addr_group_include_list_ref(addr_group),
                    zone_func, ctx, remote_ip, addr_group->include_zones,
                    addr_group->include_is_empty);
    if (exclude_all)
        return ip_included;

    /* Include or Exclude */
    return ip_included && !ip_excluded;
}

FORT_API BOOL fort_conf_app_exe_equal(PFORT_APP_ENTRY app_entry, const PVOID path, UINT32 path_len)
{
    const char *app_path = (const char *) (app_entry + 1);
    const UINT32 app_path_len = app_entry->path_len;

    if (path_len != app_path_len)
        return FALSE;

    return fort_memcmp(path, app_path, path_len) == 0;
}

FORT_API FORT_APP_FLAGS fort_conf_app_exe_find(
        const PFORT_CONF conf, const PVOID path, UINT32 path_len)
{
    FORT_APP_FLAGS app_flags;
    UINT16 count = conf->exe_apps_n;

    app_flags.v = 0;

    if (count == 0)
        return app_flags;

    const char *data = conf->data;
    const char *app_entries = (const char *) (data + conf->exe_apps_off);

    do {
        const PFORT_APP_ENTRY app_entry = (const PFORT_APP_ENTRY) app_entries;

        if (fort_conf_app_exe_equal(app_entry, path, path_len)) {
            app_flags = app_entry->flags;
            break;
        }

        app_entries += FORT_CONF_APP_ENTRY_SIZE(app_entry->path_len);
    } while (--count != 0);

    return app_flags;
}

static int fort_conf_app_prefix_cmp(PFORT_APP_ENTRY app_entry, const char *path, UINT32 path_len)
{
    const char *app_path = (const char *) (app_entry + 1);
    const UINT32 app_path_len = app_entry->path_len;

    if (path_len > app_path_len)
        path_len = app_path_len;

    return fort_memcmp(path, app_path, path_len);
}

static FORT_APP_FLAGS fort_conf_app_prefix_find(
        const PFORT_CONF conf, const char *path, UINT32 path_len)
{
    FORT_APP_FLAGS app_flags;
    const UINT16 count = conf->prefix_apps_n;

    app_flags.v = 0;

    if (count == 0)
        return app_flags;

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

        if (res < 0)
            high = mid - 1;
        else if (res > 0)
            low = mid + 1;
        else {
            app_flags = app_entry->flags;
            break;
        }
    } while (low <= high);

    return app_flags;
}

static FORT_APP_FLAGS fort_conf_app_wild_find(const PFORT_CONF conf, const char *path)
{
    FORT_APP_FLAGS app_flags;
    UINT16 count = conf->wild_apps_n;

    app_flags.v = 0;

    if (count == 0)
        return app_flags;

    const char *data = conf->data;
    const char *app_entries = (const char *) (data + conf->wild_apps_off);

    do {
        const PFORT_APP_ENTRY app_entry = (const PFORT_APP_ENTRY) app_entries;
        const WCHAR *app_path = (const WCHAR *) (app_entry + 1);
        const int res = wildmatch(app_path, (const WCHAR *) path);

        if (res == WM_MATCH) {
            app_flags = app_entry->flags;
            break;
        }

        app_entries += FORT_CONF_APP_ENTRY_SIZE(app_entry->path_len);
    } while (--count != 0);

    return app_flags;
}

FORT_API FORT_APP_FLAGS fort_conf_app_find(const PFORT_CONF conf, const PVOID path, UINT32 path_len,
        fort_conf_app_exe_find_func *exe_find_func)
{
    FORT_APP_FLAGS app_flags;

    app_flags = exe_find_func(conf, path, path_len);
    if (app_flags.v != 0)
        return app_flags;

    app_flags = fort_conf_app_prefix_find(conf, path, path_len);
    if (app_flags.v != 0)
        return app_flags;

    return fort_conf_app_wild_find(conf, path);
}

static BOOL fort_conf_app_blocked_check(const PFORT_CONF conf, INT8 *block_reason, BOOL app_found,
        BOOL app_allowed, BOOL app_blocked)
{
    *block_reason =
            app_found ? FORT_BLOCK_REASON_APP_GROUP_FOUND : FORT_BLOCK_REASON_APP_GROUP_DEFAULT;

    /* Block All */
    if (conf->flags.app_block_all)
        return !app_allowed; /* Block, if it is not explicitly allowed */

    /* Allow All */
    if (conf->flags.app_allow_all)
        return app_blocked; /* Block, if it is explicitly blocked */

    /* Block or Allow */
    if (!app_found) {
        *block_reason = FORT_BLOCK_REASON_NONE; /* Don't block or allow */
        return FALSE; /* Implicitly allow */
    }

    /* Block, if it is explicitly blocked and not allowed */
    return app_blocked && !app_allowed;
}

FORT_API BOOL fort_conf_app_blocked(
        const PFORT_CONF conf, FORT_APP_FLAGS app_flags, INT8 *block_reason)
{
    const BOOL app_found = (app_flags.v != 0);

    if (app_found && !app_flags.use_group_perm) {
        *block_reason = FORT_BLOCK_REASON_PROGRAM;
        return app_flags.blocked;
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
