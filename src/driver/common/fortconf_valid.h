#ifndef FORTCONF_VALID_H
#define FORTCONF_VALID_H

#include "fortconf.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API BOOL fort_conf_zones_valid(PCFORT_CONF_ZONES zones, UINT32 len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCONF_VALID_H
