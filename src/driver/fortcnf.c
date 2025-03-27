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
