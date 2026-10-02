/* Fort Firewall Driver Configuration Validation */

#include "fortconf_valid.h"

#include "fortcmnutl.h"

#define FORT_CONF_ADDR_GROUP_COUNT 2 /* LAN and INET */

/* Each list's level is an OR-list of lines with AND-lists of sections */
#define FORT_CONF_RULE_FILTER_LIST_DEPTH_MAX (2 * (FORT_CONF_RULE_FILTER_DEPTH_MAX + 1))

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
            && app_entry->path[path_len / sizeof(WCHAR)] == L'\0';
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

/* The data are ordered: address groups, wildcard, prefix and exe apps */
static BOOL fort_conf_data_offsets_valid(PCFORT_CONF conf, UINT32 data_len)
{
    if (conf->addr_groups_off > conf->wild_apps_off)
        return FALSE;

    if (conf->wild_apps_off > conf->prefix_apps_off)
        return FALSE;

    if (conf->prefix_apps_off > conf->exe_apps_off)
        return FALSE;

    return conf->exe_apps_off <= data_len;
}

static BOOL fort_conf_data_valid(PCFORT_CONF conf, UINT32 data_len)
{
    const char *data = conf->data;

    const UINT32 addr_groups_off = conf->addr_groups_off;
    const UINT32 wild_apps_off = conf->wild_apps_off;
    const UINT32 prefix_apps_off = conf->prefix_apps_off;
    const UINT32 exe_apps_off = conf->exe_apps_off;

    return fort_conf_addr_groups_valid(data + addr_groups_off, wild_apps_off - addr_groups_off)
            && fort_conf_app_entries_valid(
                    data + wild_apps_off, prefix_apps_off - wild_apps_off, conf->wild_apps_n)
            && fort_conf_prefix_app_entries_valid(
                    data + prefix_apps_off, exe_apps_off - prefix_apps_off, conf->prefix_apps_n)
            && fort_conf_app_entries_valid(
                    data + exe_apps_off, data_len - exe_apps_off, conf->exe_apps_n);
}

FORT_API BOOL fort_conf_io_valid(PCFORT_CONF_IO conf_io, UINT32 len)
{
    if (len < FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF)
        return FALSE;

    PCFORT_CONF conf = &conf_io->conf;

    const UINT32 data_len = len - (FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF);

    return fort_conf_data_offsets_valid(conf, data_len) && fort_conf_data_valid(conf, data_len);
}

static BOOL fort_conf_rule_filter_address_valid(const char *data, UINT32 len)
{
    return fort_conf_addr_list_valid((PCFORT_CONF_ADDR_LIST) data, len);
}

static BOOL fort_conf_rule_filter_port_valid(const char *data, UINT32 len)
{
    PCFORT_CONF_PORT_LIST port_list = (PCFORT_CONF_PORT_LIST) data;

    return len >= FORT_CONF_PORT_LIST_OFF
            && FORT_CONF_PORT_LIST_SIZE(port_list->port_n, port_list->pair_n) <= len;
}

static BOOL fort_conf_rule_filter_protocol_valid(const char *data, UINT32 len)
{
    PCFORT_CONF_PROTO_LIST proto_list = (PCFORT_CONF_PROTO_LIST) data;

    return len >= FORT_CONF_PROTO_LIST_OFF
            && FORT_CONF_PROTO_LIST_SIZE(proto_list->proto_n, proto_list->pair_n) <= len;
}

static BOOL fort_conf_rule_filter_flags_valid(const char *data, UINT32 len)
{
    UNUSED(data);

    return len >= sizeof(FORT_CONF_RULE_FILTER_FLAGS);
}

typedef BOOL (*FORT_CONF_RULE_FILTER_VALID_FUNC)(const char *data, UINT32 len);

static const FORT_CONF_RULE_FILTER_VALID_FUNC fort_conf_rule_filter_valid_funcList[] = {
    &fort_conf_rule_filter_address_valid, // FORT_RULE_FILTER_TYPE_ADDRESS,
    &fort_conf_rule_filter_port_valid, // FORT_RULE_FILTER_TYPE_PORT,
    &fort_conf_rule_filter_address_valid, // FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS,
    &fort_conf_rule_filter_port_valid, // FORT_RULE_FILTER_TYPE_LOCAL_PORT,
    &fort_conf_rule_filter_protocol_valid, // FORT_RULE_FILTER_TYPE_PROTOCOL,
    &fort_conf_rule_filter_flags_valid, // FORT_RULE_FILTER_TYPE_IP_VERSION,
    &fort_conf_rule_filter_flags_valid, // FORT_RULE_FILTER_TYPE_DIRECTION,
    &fort_conf_rule_filter_flags_valid, // FORT_RULE_FILTER_TYPE_ZONES,
    &fort_conf_rule_filter_flags_valid, // FORT_RULE_FILTER_TYPE_AREA,
    &fort_conf_rule_filter_flags_valid, // FORT_RULE_FILTER_TYPE_PROFILE,
    &fort_conf_rule_filter_flags_valid, // FORT_RULE_FILTER_TYPE_ACTION,
    &fort_conf_rule_filter_flags_valid, // FORT_RULE_FILTER_TYPE_OPTION,
    // Complex types
    &fort_conf_rule_filter_port_valid, // FORT_RULE_FILTER_TYPE_PORT_TCP,
    &fort_conf_rule_filter_port_valid, // FORT_RULE_FILTER_TYPE_PORT_UDP,
};

static_assert(
        FORT_ARRAY_SIZE(fort_conf_rule_filter_valid_funcList) == FORT_RULE_FILTER_TYPE_PORT_UDP + 1,
        "fort_conf_rule_filter_valid_funcList size mismatch");

static BOOL fort_conf_rule_filter_values_valid(int filter_type, const char *data, UINT32 len)
{
    const UINT32 index = (UINT32) filter_type;

    const FORT_CONF_RULE_FILTER_VALID_FUNC func =
            (index < FORT_ARRAY_SIZE(fort_conf_rule_filter_valid_funcList))
            ? fort_conf_rule_filter_valid_funcList[index]
            : &fort_conf_rule_filter_flags_valid;

    return func(data, len);
}

static BOOL fort_conf_rule_filter_valid(
        PCFORT_CONF_RULE_FILTER rule_filter, UINT32 len, int list_depth);

static BOOL fort_conf_rule_filter_list_valid(const char *data, UINT32 data_len, int list_depth)
{
    if (++list_depth > FORT_CONF_RULE_FILTER_LIST_DEPTH_MAX)
        return FALSE;

    /* The list is not empty and its filters fill it exactly */
    do {
        PCFORT_CONF_RULE_FILTER sub_filter = (PCFORT_CONF_RULE_FILTER) data;

        if (!fort_conf_rule_filter_valid(sub_filter, data_len, list_depth))
            return FALSE;

        data += sub_filter->size;
        data_len -= sub_filter->size;
    } while (data_len != 0);

    return TRUE;
}

static BOOL fort_conf_rule_filter_valid(
        PCFORT_CONF_RULE_FILTER rule_filter, UINT32 len, int list_depth)
{
    if (len < sizeof(FORT_CONF_RULE_FILTER))
        return FALSE;

    const UINT32 size = rule_filter->size;
    if (size < sizeof(FORT_CONF_RULE_FILTER) || size > len)
        return FALSE;

    const char *data = (const char *) (rule_filter + 1);
    const UINT32 data_len = size - sizeof(FORT_CONF_RULE_FILTER);

    const int filter_type = rule_filter->type;

    if (filter_type == FORT_RULE_FILTER_TYPE_LIST_OR
            || filter_type == FORT_RULE_FILTER_TYPE_LIST_AND) {

        return fort_conf_rule_filter_list_valid(data, data_len, list_depth);
    }

    if (rule_filter->is_empty)
        return TRUE;

    return fort_conf_rule_filter_values_valid(filter_type, data, data_len);
}

static BOOL fort_conf_rule_valid(
        PCFORT_CONF_RULES_RT rules_rt, UINT32 rule_off, UINT32 data_len, UINT32 *rule_end)
{
    const UINT32 len = data_len - rule_off;

    if (len < sizeof(FORT_CONF_RULE))
        return FALSE;

    PCFORT_CONF_RULE rule = (PCFORT_CONF_RULE) (rules_rt->rules_data + rule_off);

    UINT32 rule_size = FORT_CONF_RULE_SIZE(rule);
    if (rule_size > len)
        return FALSE;

    if (rule->has_filters) {
        PCFORT_CONF_RULE_FILTER rule_filter = (PCFORT_CONF_RULE_FILTER) ((PCCH) rule + rule_size);

        if (!fort_conf_rule_filter_valid(rule_filter, len - rule_size, /*list_depth=*/0))
            return FALSE;

        rule_size += rule_filter->size;
    }

    *rule_end = rule_off + rule_size;

    return TRUE;
}

static BOOL fort_conf_rule_id_valid(
        PCFORT_CONF_RULES_RT rules_rt, UINT16 rule_id, UINT32 data_len, UINT32 *rules_end)
{
    const UINT32 rule_off = rules_rt->rule_offsets[rule_id];
    if (rule_off == 0)
        return TRUE; /* absent rule */

    if (rule_off < *rules_end || rule_off > data_len)
        return FALSE;

    return fort_conf_rule_valid(rules_rt, rule_off, data_len, rules_end);
}

FORT_API BOOL fort_conf_rules_valid(PCFORT_CONF_RULES rules, UINT32 len)
{
    if (len < FORT_CONF_RULES_DATA_OFF)
        return FALSE;

    const UINT16 max_rule_id = rules->max_rule_id;
    if (max_rule_id > FORT_CONF_RULE_ID_MAX)
        return FALSE;

    const UINT32 data_len = len - FORT_CONF_RULES_DATA_OFF;

    UINT32 rules_end = FORT_CONF_RULES_OFFSETS_SIZE(max_rule_id);
    if (rules_end > data_len)
        return FALSE;

    const FORT_CONF_RULES_RT rules_rt = fort_conf_rules_rt_make(rules, /*zones=*/NULL);

    /* The rules follow their offsets ordered by id, so SETRULEFLAG changes only its rule */
    for (UINT16 rule_id = 1; rule_id <= max_rule_id; ++rule_id) {
        if (!fort_conf_rule_id_valid(&rules_rt, rule_id, data_len, &rules_end))
            return FALSE;
    }

    return TRUE;
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
