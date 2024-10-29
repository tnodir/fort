/* Fort Firewall Driver Configuration */

#include "fortconf.h"

#include <assert.h>

#include "fort_wildmatch.h"
#include "fortdef.h"

static_assert(sizeof(ip6_addr_t) == 16, "ip6_addr_t size mismatch");

static_assert(sizeof(FORT_CONF_FLAGS) == sizeof(UINT64), "FORT_CONF_FLAGS size mismatch");
static_assert(sizeof(FORT_CONF_RULE_EXPR) == sizeof(UINT32), "FORT_CONF_RULE_EXPR size mismatch");
static_assert(sizeof(FORT_CONF_RULE) == sizeof(UINT16), "FORT_CONF_RULE size mismatch");
static_assert(sizeof(FORT_TRAF) == sizeof(UINT64), "FORT_TRAF size mismatch");
static_assert(sizeof(FORT_APP_FLAGS) == sizeof(UINT16), "FORT_APP_FLAGS size mismatch");
static_assert(sizeof(FORT_APP_DATA) == 2 * sizeof(UINT32), "FORT_APP_DATA size mismatch");

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

FORT_API BOOL fort_conf_ip_inlist(
        const UINT32 *ip, const PFORT_CONF_ADDR_LIST addr_list, BOOL isIPv6)
{
    if (isIPv6) {
        const ip6_addr_t *ip6 = (const ip6_addr_t *) ip;
        const PFORT_CONF_ADDR_LIST addr6_list = (const PFORT_CONF_ADDR_LIST)((const PCHAR) addr_list
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

static BOOL fort_conf_ip_included_check(const PFORT_CONF_ADDR_LIST addr_list,
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

static FORT_APP_DATA fort_conf_app_find_loop(const PFORT_CONF conf, PCFORT_APP_PATH path,
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

FORT_API FORT_APP_DATA fort_conf_app_exe_find(
        const PFORT_CONF conf, PVOID context, PCFORT_APP_PATH path)
{
    UNUSED(context);

    return fort_conf_app_find_loop(
            conf, path, conf->exe_apps_off, conf->exe_apps_n, fort_conf_app_exe_equal);
}

inline static FORT_APP_DATA fort_conf_app_wild_find(const PFORT_CONF conf, PCFORT_APP_PATH path)
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

inline static FORT_APP_DATA fort_conf_app_prefix_find(const PFORT_CONF conf, PCFORT_APP_PATH path)
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

FORT_API FORT_APP_DATA fort_conf_app_find(const PFORT_CONF conf, PCFORT_APP_PATH path,
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

FORT_API PCWSTR fort_conf_service_sid_name_find(
        PCFORT_SERVICE_SID_LIST service_sids, const char *sidBytes)
{
    if (service_sids == NULL)
        return NULL;

    const char *data = service_sids->data;
    const int services_n = service_sids->services_n;

    const int sid_index = fort_conf_blob_index(data, sidBytes, FORT_SERVICE_SID_SIZE, services_n);
    if (sid_index < 0)
        return NULL;

    const UINT16 name_index =
            ((UINT16 *) (data + FORT_SERVICE_SID_LIST_SID_NAME_INDEXES_OFF(services_n)))[sid_index];

    const UINT32 name_off =
            ((UINT32 *) (data + FORT_SERVICE_SID_LIST_NAMES_HEADER_OFF(services_n)))[name_index];

    const int names_n = service_sids->names_n;
    const char *names_data = data + FORT_SERVICE_SID_LIST_NAMES_OFF(services_n, names_n);

    PCWSTR name = (PCWSTR) (names_data + name_off);

    return name;
}
