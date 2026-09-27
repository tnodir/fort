#ifndef FORTCNF_GROUP_H
#define FORTCNF_GROUP_H

#include "fortcnf.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API PFORT_CONF_GROUPS fort_conf_groups_new(PCFORT_CONF_GROUPS groups, ULONG len);

FORT_API void fort_conf_groups_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_GROUPS groups);

FORT_API void fort_conf_group_flags_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_GROUP_FLAGS group_flags);

FORT_API BOOL fort_devconf_groups_mask_blocked(PFORT_DEVICE_CONF device_conf, UINT32 groups_mask);

FORT_API UINT16 fort_devconf_groups_rules_conn_filtered(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_META_CONN conn, UINT32 groups_mask);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_GROUP_H
