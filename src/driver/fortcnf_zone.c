/* Fort Firewall Configuration: Zones */

#include "fortcnf_zone.h"

FORT_API PFORT_CONF_ZONES fort_conf_zones_new(PCFORT_CONF_ZONES zones, ULONG len)
{
    return fort_conf_mem_alloc(zones, len);
}

FORT_API void fort_conf_zones_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_ZONES zones)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    {
        fort_conf_mem_free(device_conf->zones);
        device_conf->zones = zones;
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

inline static void fort_conf_zone_flag_set_locked(
        PFORT_CONF_ZONES zones, PCFORT_CONF_ZONE_FLAG zone_flag)
{
    const UINT32 zone_mask = (1u << (zone_flag->zone_id - 1));

    if (zone_flag->enabled) {
        zones->enabled_mask |= zone_mask;
    } else {
        zones->enabled_mask &= ~zone_mask;
    }
}

FORT_API void fort_conf_zone_flag_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_ZONE_FLAG zone_flag)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_ZONES zones = device_conf->zones;
    if (zones != NULL) {
        fort_conf_zone_flag_set_locked(zones, zone_flag);
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API BOOL fort_devconf_zones_ip_included(PFORT_DEVICE_CONF device_conf,
        PCFORT_CONF_META_CONN conn, UCHAR *zone_id, UINT32 zones_mask)
{
    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockShared(&device_conf->lock);
    PCFORT_CONF_ZONES zones = device_conf->zones;
    if (zones != NULL) {
        res = fort_conf_zones_ip_included(zones, conn, zone_id, zones_mask);
    }
    ExReleaseSpinLockShared(&device_conf->lock, oldIrql);

    return res;
}

FORT_API BOOL fort_devconf_zones_conn_filtered(PFORT_DEVICE_CONF device_conf,
        PCFORT_CONF_META_CONN conn, PFORT_CONF_ZONES_CONN_FILTERED_OPT opt)
{
    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockShared(&device_conf->lock);
    PCFORT_CONF_ZONES zones = device_conf->zones;
    if (zones != NULL) {
        res = fort_conf_zones_conn_filtered(zones, conn, opt);
    }
    ExReleaseSpinLockShared(&device_conf->lock, oldIrql);

    return res;
}
