#ifndef FORTCNF_ZONE_H
#define FORTCNF_ZONE_H

#include "fortcnf.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API PFORT_CONF_ZONES fort_conf_zones_new(PCFORT_CONF_ZONES zones, ULONG len);

FORT_API void fort_conf_zones_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_ZONES zones);

FORT_API void fort_conf_zone_flag_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_ZONE_FLAG zone_flag);

FORT_API BOOL fort_devconf_zones_ip_included(PFORT_DEVICE_CONF device_conf,
        PCFORT_CONF_META_CONN conn, UCHAR *zone_id, UINT32 zones_mask);

FORT_API BOOL fort_devconf_zones_conn_filtered(PFORT_DEVICE_CONF device_conf,
        PCFORT_CONF_META_CONN conn, PFORT_CONF_ZONES_CONN_FILTERED_OPT opt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_ZONE_H
