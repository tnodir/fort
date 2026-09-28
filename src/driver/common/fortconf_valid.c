/* Fort Firewall Driver Configuration Validation */

#include "fortconf_valid.h"

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
