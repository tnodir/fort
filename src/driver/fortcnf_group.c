/* Fort Firewall Configuration: Groups */

#include "fortcnf_group.h"

FORT_API PFORT_CONF_GROUPS fort_conf_groups_new(PCFORT_CONF_GROUPS groups, ULONG len)
{
    return fort_conf_mem_alloc(groups, len);
}

FORT_API void fort_conf_groups_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_GROUPS groups)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    {
        fort_conf_mem_free(device_conf->groups);
        device_conf->groups = groups;
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API void fort_conf_group_flags_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_GROUP_FLAGS group_flags)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_GROUPS groups = device_conf->groups;
    if (groups != NULL) {
        groups->enabled_mask = group_flags->enabled_mask;
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API BOOL fort_devconf_groups_mask_blocked(PFORT_DEVICE_CONF device_conf, UINT32 groups_mask)
{
    if (groups_mask == 0)
        return FALSE; /* the App is not in any Group: don't lock */

    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockShared(&device_conf->lock);
    PCFORT_CONF_GROUPS groups = device_conf->groups;
    if (groups != NULL) {
        res = fort_conf_groups_mask_blocked(groups, groups_mask);
    }
    ExReleaseSpinLockShared(&device_conf->lock, oldIrql);

    return res;
}

FORT_API UINT16 fort_devconf_groups_rules_conn_filtered(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_META_CONN conn, UINT32 groups_mask)
{
    if (groups_mask == 0)
        return 0; /* the App is not in any Group: don't lock */

    UINT16 rule_id = 0;

    /* The Groups and Rules are under the same lock */
    KIRQL oldIrql = ExAcquireSpinLockShared(&device_conf->lock);
    PCFORT_CONF_GROUPS groups = device_conf->groups;
    PCFORT_CONF_RULES rules = device_conf->rules;
    if (groups != NULL && rules != NULL) {
        rule_id = fort_conf_groups_rules_conn_filtered(
                groups, rules, device_conf->zones, conn, groups_mask);
    }
    ExReleaseSpinLockShared(&device_conf->lock, oldIrql);

    return rule_id;
}
