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

inline static void fort_conf_group_flag_set_locked(
        PFORT_CONF_GROUPS groups, PCFORT_CONF_GROUP_FLAG group_flag)
{
    const UINT32 group_mask = (1u << (group_flag->group_id - 1));

    if (group_flag->enabled) {
        groups->enabled_mask |= group_mask;
    } else {
        groups->enabled_mask &= ~group_mask;
    }
}

FORT_API void fort_conf_group_flag_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_GROUP_FLAG group_flag)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_GROUPS groups = device_conf->groups;
    if (groups != NULL) {
        fort_conf_group_flag_set_locked(groups, group_flag);
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API BOOL fort_devconf_groups_mask_included(PFORT_DEVICE_CONF device_conf, UINT32 groups_mask)
{
    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockShared(&device_conf->lock);
    PCFORT_CONF_GROUPS groups = device_conf->groups;
    if (groups != NULL) {
        res = fort_conf_groups_mask_included(groups, groups_mask);
    }
    ExReleaseSpinLockShared(&device_conf->lock, oldIrql);

    return res;
}
