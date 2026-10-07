#ifndef FORTCNF_RULE_H
#define FORTCNF_RULE_H

#include "fortcnf.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API PFORT_CONF_RULES fort_conf_rules_new(PCFORT_CONF_RULES rules, ULONG len);

FORT_API void fort_conf_rules_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_RULES rules);

FORT_API void fort_conf_rule_flag_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_RULE_FLAG rule_flag);

FORT_API void fort_conf_periods_set(PFORT_DEVICE_CONF device_conf, PCFORT_CONF_PERIODS periods);

/* Call under the device_conf's lock */
FORT_API FORT_CONF_RULES_RT fort_devconf_rules_rt_make(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_RULES rules);

FORT_API BOOL fort_devconf_rules_conn_filtered(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_META_CONN conn, UINT16 rule_id);

FORT_API UINT16 fort_devconf_rules_glob_conn_filtered(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_META_CONN conn, BOOL is_post);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_RULE_H
