#ifndef COMMON_H
#define COMMON_H

#if defined(FORT_DRIVER)
#    define NDIS_WDM 1
#    define NDIS630  1

#    define WIN9X_COMPAT_SPINLOCK /* XXX: Support Windows 7: KeInitializeSpinLock() */
#    define POOL_NX_OPTIN         1 /* Enhanced protection of NX pool */

#    include <wdm.h>
#    include <fwpmk.h>
#    include <fwpsk.h>
#    include <stddef.h>
#    include <ntrxdef.h>
#else
#    define _WIN32_WINNT 0x0601
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    include <winioctl.h>
#    include <fwpmu.h>
#endif

#if !defined(FORT_API)
#    if defined(FORT_AMALG)
#        define FORT_API static
#    else
#        define FORT_API extern
#    endif
#endif

#define UNUSED(p) ((void) (p))

#ifndef NT_SUCCESS
#    define NT_SUCCESS(status) ((LONG)(status) >= 0)
#endif

#define FORT_STATUS_USER_ERROR STATUS_INVALID_PARAMETER
#define FORT_ERROR_USER_ERROR  ERROR_INVALID_PARAMETER

/* Convert system time to seconds since 1970 */
#define SECSPERDAY 86400
#define SECS_1601_TO_1970                                                                          \
    ((369 * 365 + 89) * (INT64) SECSPERDAY) /* 1601 to 1970 is 369 years plus 89 leap days */
#define fort_system_to_unix_time(system_time) ((system_time) / 10000000 - SECS_1601_TO_1970)

#endif // COMMON_H
