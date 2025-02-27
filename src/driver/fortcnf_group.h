#ifndef FORTCNF_GROUP_H
#define FORTCNF_GROUP_H

#include "fortcnf.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API PFORT_CONF_GROUPS fort_conf_groups_new(PCFORT_CONF_GROUPS groups, ULONG len);

FORT_API void fort_conf_groups_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_GROUPS groups);

FORT_API void fort_conf_group_flag_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_GROUP_FLAG group_flag);

FORT_API BOOL fort_devconf_groups_mask_included(PFORT_DEVICE_CONF device_conf, UINT32 groups_mask);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_GROUP_H
