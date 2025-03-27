/* Fort Firewall Configuration: Rules */

#include "fortcnf_rule.h"

FORT_API PFORT_CONF_RULES fort_conf_rules_new(PCFORT_CONF_RULES rules, ULONG len)
{
    return fort_conf_mem_alloc(rules, len);
}

FORT_API void fort_conf_rules_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_RULES rules)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    {
        fort_conf_mem_free(device_conf->rules);
        device_conf->rules = rules;

        if (rules != NULL) {
            device_conf->rules_glob = rules->glob;
        } else {
            const FORT_CONF_RULES_GLOB rules_glob = { 0 };
            device_conf->rules_glob = rules_glob;
        }
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

inline static void fort_conf_rule_flag_set_locked(
        PFORT_CONF_RULES rules, PCFORT_CONF_RULE_FLAG rule_flag)
{
    if (rule_flag->rule_id > rules->max_rule_id)
        return;

    const FORT_CONF_RULES_RT rules_rt = fort_conf_rules_rt_make(rules, /*zones=*/NULL);
    PFORT_CONF_RULE rule = fort_conf_rules_rt_rule(&rules_rt, rule_flag->rule_id);

    rule->enabled = rule_flag->enabled;
}

FORT_API void fort_conf_rule_flag_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_RULE_FLAG rule_flag)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_RULES rules = device_conf->rules;
    if (rules != NULL) {
        fort_conf_rule_flag_set_locked(rules, rule_flag);
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API BOOL fort_devconf_rules_conn_filtered(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_META_CONN conn, UINT16 rule_id)
{
    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_RULES rules = device_conf->rules;
    if (rules != NULL) {
        res = fort_conf_rules_conn_filtered(rules, device_conf->zones, conn, rule_id);
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);

    return res;
}
