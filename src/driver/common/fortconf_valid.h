#ifndef FORTCONF_VALID_H
#define FORTCONF_VALID_H

#include "fortconf.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API BOOL fort_conf_app_entry_valid(PCFORT_APP_ENTRY app_entry, UINT32 len);

FORT_API BOOL fort_conf_io_valid(PCFORT_CONF_IO conf_io, UINT32 len);

FORT_API BOOL fort_conf_rules_valid(PCFORT_CONF_RULES rules, UINT32 len);

FORT_API BOOL fort_conf_zones_valid(PCFORT_CONF_ZONES zones, UINT32 len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCONF_VALID_H
