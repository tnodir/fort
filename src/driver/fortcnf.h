#ifndef FORTCNF_H
#define FORTCNF_H

#include "fortdrv.h"

#include "common/fortconf.h"
#include "fortpool.h"
#include "forttds.h"

#define FORT_SVCHOST_PREFIX L"\\svchost\\"
#define FORT_SVCHOST_PREFIX_SIZE                                                                   \
    (sizeof(FORT_SVCHOST_PREFIX) - sizeof(WCHAR)) /* skip terminating zero */

typedef struct fort_conf_ref
{
    UINT32 volatile refcount;

    FORT_POOL_LIST pool_list;
    tommy_list free_nodes;

    tommy_arrayof exe_nodes;
    tommy_hashdyn exe_map;

    EX_SPIN_LOCK conf_lock;

    FORT_CONF conf;
} FORT_CONF_REF, *PFORT_CONF_REF;

#define FORT_DEVICE_BOOT_FILTER   0x01
#define FORT_DEVICE_STEALTH_MODE  0x02
#define FORT_DEVICE_FILTER_LOCALS 0x04
#define FORT_DEVICE_BOOT_CONF_MASK                                                                 \
    (FORT_DEVICE_BOOT_FILTER | FORT_DEVICE_FILTER_LOCALS | FORT_DEVICE_STEALTH_MODE)

#define FORT_DEVICE_TRACE_EVENTS        0x08
#define FORT_DEVICE_IS_OPENED           0x10
#define FORT_DEVICE_IS_VALIDATED        0x20
#define FORT_DEVICE_POWER_OFF           0x40
#define FORT_DEVICE_SHUTDOWN_REGISTERED 0x80

typedef struct fort_device_conf
{
    UINT16 volatile flags;

    FORT_CONF_RULES_GLOB volatile rules_glob;

    FORT_CONF_FLAGS volatile conf_flags;
    PFORT_CONF_REF volatile ref;
    KSPIN_LOCK ref_lock;

    PFORT_CONF_ZONES zones;
    PFORT_CONF_RULES rules;

    EX_SPIN_LOCK lock;
} FORT_DEVICE_CONF, *PFORT_DEVICE_CONF;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API PVOID fort_conf_mem_alloc(const void *src, ULONG len);

FORT_API void fort_conf_mem_free(PVOID p);

FORT_API void fort_device_conf_open(PFORT_DEVICE_CONF device_conf);

FORT_API UINT16 fort_device_flag_set(PFORT_DEVICE_CONF device_conf, UINT16 flag, BOOL on);

FORT_API UINT16 fort_device_flags(PFORT_DEVICE_CONF device_conf);

FORT_API UINT16 fort_device_flag(PFORT_DEVICE_CONF device_conf, UINT16 flag);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_H
