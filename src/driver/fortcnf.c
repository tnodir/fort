/* Fort Firewall Configuration */

#include "fortcnf.h"

#define FORT_DEVICE_CONF_POOL_TAG 'CwfF'

FORT_API PVOID fort_conf_mem_alloc(const void *src, ULONG len)
{
    PVOID p = fort_mem_alloc(len, FORT_DEVICE_CONF_POOL_TAG);
    if (p != NULL) {
        RtlCopyMemory(p, src, len);
    }
    return p;
}

FORT_API void fort_conf_mem_free(PVOID p)
{
    if (p != NULL) {
        fort_mem_free(p, FORT_DEVICE_CONF_POOL_TAG);
    }
}

FORT_API void fort_device_conf_open(PFORT_DEVICE_CONF device_conf)
{
    KeInitializeSpinLock(&device_conf->ref_lock);
}

FORT_API UINT16 fort_device_flag_set(PFORT_DEVICE_CONF device_conf, UINT16 flag, BOOL on)
{
    return on ? InterlockedOr16(&device_conf->flags, flag)
              : InterlockedAnd16(&device_conf->flags, ~flag);
}

FORT_API UINT16 fort_device_flags(PFORT_DEVICE_CONF device_conf)
{
    return fort_device_flag_set(device_conf, 0, TRUE);
}

FORT_API UINT16 fort_device_flag(PFORT_DEVICE_CONF device_conf, UINT16 flag)
{
    return fort_device_flags(device_conf) & flag;
}

FORT_API UINT16 fort_device_prov_flags_by_conf(const FORT_CONF_FLAGS conf_flags)
{
    return FORT_DEVICE_PROVIDER | (conf_flags.boot_filter ? FORT_DEVICE_BOOT_FILTER : 0)
            | (conf_flags.stealth_mode ? FORT_DEVICE_STEALTH_MODE : 0)
            | (conf_flags.filter_locals ? FORT_DEVICE_FILTER_LOCALS : 0)
            | (conf_flags.log_stat ? FORT_DEVICE_FLOW_FILTERS : 0);
}

FORT_API void fort_device_prov_flags_set(PFORT_DEVICE_CONF device_conf, UINT16 prov_flags)
{
    fort_device_flag_set(device_conf, (FORT_DEVICE_PROV_MASK & ~prov_flags), FALSE);
    fort_device_flag_set(device_conf, prov_flags, TRUE);
}

FORT_API FORT_CONF_FLAGS fort_device_conf_flags(PFORT_DEVICE_CONF device_conf)
{
    const FORT_CONF_FLAGS conf_flags = {
        .v = InterlockedCompareExchange(&device_conf->conf_flags.v, 0, 0),
    };

    return conf_flags;
}

FORT_API void fort_device_conf_flags_set(PFORT_DEVICE_CONF device_conf, FORT_CONF_FLAGS conf_flags)
{
    InterlockedExchange(&device_conf->conf_flags.v, conf_flags.v);
}
