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

#define FORT_DEVICE_BOOT_FILTER         0x01
#define FORT_DEVICE_BOOT_FILTER_LOCALS  0x02
#define FORT_DEVICE_BOOT_MASK           (FORT_DEVICE_BOOT_FILTER | FORT_DEVICE_BOOT_FILTER_LOCALS)
#define FORT_DEVICE_IS_OPENED           0x10
#define FORT_DEVICE_IS_VALIDATED        0x20
#define FORT_DEVICE_POWER_OFF           0x40
#define FORT_DEVICE_SHUTDOWN_REGISTERED 0x80

typedef struct fort_device_conf
{
    UINT16 volatile flags;

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

FORT_API void fort_device_conf_open(PFORT_DEVICE_CONF device_conf);

FORT_API UINT16 fort_device_flag_set(PFORT_DEVICE_CONF device_conf, UINT16 flag, BOOL on);

FORT_API UINT16 fort_device_flag(PFORT_DEVICE_CONF device_conf, UINT16 flag);

FORT_API FORT_APP_DATA fort_conf_exe_find(
        const PFORT_CONF conf, PVOID context, PCFORT_APP_PATH path);

FORT_API NTSTATUS fort_conf_ref_exe_add_path(
        PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path);

FORT_API NTSTATUS fort_conf_ref_exe_add_entry(
        PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY entry, BOOL locked);

FORT_API void fort_conf_ref_exe_del_entry(PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY entry);

FORT_API PFORT_CONF_REF fort_conf_ref_new(const PFORT_CONF conf, ULONG len);

FORT_API void fort_conf_ref_put(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref);

FORT_API PFORT_CONF_REF fort_conf_ref_take(PFORT_DEVICE_CONF device_conf);

FORT_API FORT_CONF_FLAGS fort_conf_ref_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref);

FORT_API FORT_CONF_FLAGS fort_conf_ref_flags_set(
        PFORT_DEVICE_CONF device_conf, const FORT_CONF_FLAGS conf_flags);

FORT_API PFORT_CONF_ZONES fort_conf_zones_new(PFORT_CONF_ZONES zones, ULONG len);

FORT_API void fort_conf_zones_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_ZONES zones);

FORT_API void fort_conf_zone_flag_set(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_ZONE_FLAG zone_flag);

FORT_API BOOL fort_conf_zones_ip_included(
        PFORT_DEVICE_CONF device_conf, UINT32 zones_mask, const ip_addr_t remote_ip, BOOL isIPv6);

FORT_API void fort_conf_rules_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_RULES rules);

FORT_API void fort_conf_rule_flag_set(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_RULE_FLAG rule_flag);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_H
