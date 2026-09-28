/* Fort Firewall Driver Configuration Validation */

#include "fortconf_valid.h"

#define FORT_CONF_ADDR_GROUP_COUNT 2 /* LAN and INET */

static UINT32 fort_conf_addr_list_part_size(
        PCFORT_CONF_ADDR_LIST addr_list, UINT32 len, BOOL isIPv6)
{
    if (len < FORT_CONF_ADDR_LIST_OFF)
        return 0;

    const UINT32 ip_n = addr_list->ip_n;
    const UINT32 pair_n = addr_list->pair_n;

    if (ip_n > FORT_CONF_IP_MAX || pair_n > FORT_CONF_IP_MAX)
        return 0;

    const UINT32 size = (UINT32) (isIPv6 ? FORT_CONF_ADDR6_LIST_SIZE(ip_n, pair_n)
                                         : FORT_CONF_ADDR4_LIST_SIZE(ip_n, pair_n));

    return (size <= len) ? size : 0;
}

static BOOL fort_conf_addr_list_valid(PCFORT_CONF_ADDR_LIST addr_list, UINT32 len)
{
    const UINT32 addr4_size = fort_conf_addr_list_part_size(addr_list, len, /*isIPv6=*/FALSE);
    if (addr4_size == 0)
        return FALSE;

    PCFORT_CONF_ADDR_LIST addr6_list = (PCFORT_CONF_ADDR_LIST) ((PCCH) addr_list + addr4_size);

    return fort_conf_addr_list_part_size(addr6_list, len - addr4_size, /*isIPv6=*/TRUE) != 0;
}

static BOOL fort_conf_addr_group_valid(PCFORT_CONF_ADDR_GROUP addr_group, UINT32 len)
{
    if (len < FORT_CONF_ADDR_GROUP_OFF)
        return FALSE;

    const UINT32 data_len = len - FORT_CONF_ADDR_GROUP_OFF;
    const UINT32 exclude_off = addr_group->exclude_off;

    if (exclude_off > data_len)
        return FALSE;

    if (!addr_group->include_is_empty
            && !fort_conf_addr_list_valid(
                    fort_conf_addr_group_include_list_ref(addr_group), exclude_off))
        return FALSE;

    if (!addr_group->exclude_is_empty
            && !fort_conf_addr_list_valid(
                    fort_conf_addr_group_exclude_list_ref(addr_group), data_len - exclude_off))
        return FALSE;

    return TRUE;
}

static BOOL fort_conf_addr_groups_valid(const char *data, UINT32 len)
{
    if (len < FORT_CONF_ADDR_GROUP_COUNT * sizeof(UINT32))
        return FALSE;

    const UINT32 *addr_group_offsets = (const UINT32 *) data;

    for (int i = 0; i < FORT_CONF_ADDR_GROUP_COUNT; ++i) {
        const UINT32 addr_group_off = addr_group_offsets[i];
        if (addr_group_off > len)
            return FALSE;

        PCFORT_CONF_ADDR_GROUP addr_group = (PCFORT_CONF_ADDR_GROUP) (data + addr_group_off);

        if (!fort_conf_addr_group_valid(addr_group, len - addr_group_off))
            return FALSE;
    }

    return TRUE;
}

FORT_API BOOL fort_conf_app_entry_valid(PCFORT_APP_ENTRY app_entry, UINT32 len)
{
    if (len < FORT_CONF_APP_ENTRY_PATH_OFF)
        return FALSE;

    const UINT16 path_len = app_entry->path_len;

    return FORT_CONF_APP_ENTRY_SIZE(path_len) <= len
            && app_entry->path[path_len / sizeof(WCHAR)] == L'\0'
            && app_entry->app_data.group_index < FORT_CONF_GROUP_MAX;
}

static BOOL fort_conf_app_entries_valid(const char *data, UINT32 len, UINT16 count)
{
    while (count-- > 0) {
        PCFORT_APP_ENTRY app_entry = (PCFORT_APP_ENTRY) data;

        if (!fort_conf_app_entry_valid(app_entry, len))
            return FALSE;

        const UINT32 entry_size = FORT_CONF_APP_ENTRY_SIZE(app_entry->path_len);

        data += entry_size;
        len -= entry_size;
    }

    return TRUE;
}

static BOOL fort_conf_prefix_app_entries_valid(const char *data, UINT32 len, UINT16 count)
{
    if (count == 0)
        return TRUE;

    const UINT32 offsets_size = FORT_CONF_STR_HEADER_SIZE(count);
    if (offsets_size > len)
        return FALSE;

    const UINT32 *app_offsets = (const UINT32 *) data;
    const char *app_entries = data + offsets_size;
    const UINT32 entries_len = len - offsets_size;

    for (int i = 0; i < count; ++i) {
        const UINT32 app_off = app_offsets[i];
        if (app_off > entries_len)
            return FALSE;

        PCFORT_APP_ENTRY app_entry = (PCFORT_APP_ENTRY) (app_entries + app_off);

        if (!fort_conf_app_entry_valid(app_entry, entries_len - app_off))
            return FALSE;
    }

    return TRUE;
}

FORT_API BOOL fort_conf_io_valid(PCFORT_CONF_IO conf_io, UINT32 len)
{
    if (len < FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF)
        return FALSE;

    PCFORT_CONF conf = &conf_io->conf;

    const char *data = conf->data;
    const UINT32 data_len = len - (FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF);

    const UINT32 addr_groups_off = conf->addr_groups_off;
    const UINT32 wild_apps_off = conf->wild_apps_off;
    const UINT32 prefix_apps_off = conf->prefix_apps_off;
    const UINT32 exe_apps_off = conf->exe_apps_off;

    /* The data are ordered: address groups, wildcard, prefix and exe apps */
    if (!(addr_groups_off <= wild_apps_off && wild_apps_off <= prefix_apps_off
                && prefix_apps_off <= exe_apps_off && exe_apps_off <= data_len))
        return FALSE;

    return fort_conf_addr_groups_valid(data + addr_groups_off, wild_apps_off - addr_groups_off)
            && fort_conf_app_entries_valid(
                    data + wild_apps_off, prefix_apps_off - wild_apps_off, conf->wild_apps_n)
            && fort_conf_prefix_app_entries_valid(
                    data + prefix_apps_off, exe_apps_off - prefix_apps_off, conf->prefix_apps_n)
            && fort_conf_app_entries_valid(
                    data + exe_apps_off, data_len - exe_apps_off, conf->exe_apps_n);
}

FORT_API BOOL fort_conf_zones_valid(PCFORT_CONF_ZONES zones, UINT32 len)
{
    if (len < FORT_CONF_ZONES_DATA_OFF)
        return FALSE;

    const UINT32 data_len = len - FORT_CONF_ZONES_DATA_OFF;

    UINT32 zones_mask = zones->mask;

    while (zones_mask != 0) {
        const int zone_index = fort_bit_scan_forward(zones_mask);

        const UINT32 addr_off = zones->addr_off[zone_index];
        if (addr_off > data_len)
            return FALSE;

        PCFORT_CONF_ADDR_LIST addr_list = (PCFORT_CONF_ADDR_LIST) &zones->data[addr_off];

        if (!fort_conf_addr_list_valid(addr_list, data_len - addr_off))
            return FALSE;

        zones_mask ^= (1u << zone_index);
    }

    return TRUE;
}
