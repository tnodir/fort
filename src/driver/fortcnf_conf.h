#ifndef FORTCNF_CONF_H
#define FORTCNF_CONF_H

#include "fortcnf.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API FORT_APP_DATA fort_conf_exe_find(PCFORT_CONF conf, PVOID context, PCFORT_APP_PATH path);

FORT_API NTSTATUS fort_conf_ref_exe_add_path(
        PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path);

FORT_API NTSTATUS fort_conf_ref_exe_add_entry(
        PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY entry, BOOL locked);

FORT_API void fort_conf_ref_exe_del_entry(PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY entry);

FORT_API PFORT_CONF_REF fort_conf_ref_new(PCFORT_CONF conf, ULONG len);

FORT_API void fort_conf_ref_put(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref);

FORT_API PFORT_CONF_REF fort_conf_ref_take(PFORT_DEVICE_CONF device_conf);

FORT_API FORT_CONF_FLAGS fort_conf_ref_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref);

FORT_API FORT_CONF_FLAGS fort_conf_ref_flags_set(
        PFORT_DEVICE_CONF device_conf, const FORT_CONF_FLAGS conf_flags);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_CONF_H
