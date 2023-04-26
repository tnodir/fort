#ifndef COMMON_H
#define COMMON_H

#include "common_types.h"

#if defined(FORT_DRIVER)
#    define NDIS_WDM 1
#    define NDIS630  1

#    if defined(FORT_WIN7_COMPAT)
#        define WIN9X_COMPAT_SPINLOCK /* XXX: Support Windows 7: KeInitializeSpinLock() */
#        define POOL_NX_OPTIN         1 /* Enhanced protection of NX pool */
#    else
#        define POOL_NX_OPTIN_AUTO 1 /* Enhanced protection of NX pool */
#    endif

#    define _KRPCENV_ /* To include winerror.h */

#    include <ntifs.h> /* Before ntddk.h */

#    include <ntddk.h>
#    include <winerror.h>

#    include <fwpmk.h>
#    include <fwpsk.h>
#    include <ntrxdef.h>
#    include <stddef.h>
#else
#    undef _WIN32_WINNT
#    define _WIN32_WINNT 0x0603
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>

#    include <fwpmu.h>
#    include <stddef.h>
#    include <winioctl.h>
#endif

#if !defined(FORT_API)
#    if defined(FORT_AMALG)
#        define FORT_API static
#    else
#        define FORT_API extern
#    endif
#endif

#define FORT_BIG_ENDIAN 0

#if defined(FORT_DRIVER)
#    define LOG(...) DbgPrintEx(DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL, "FORT: " __VA_ARGS__)
#else
#    define LOG(...)
#endif

#ifndef NT_SUCCESS
#    define NT_SUCCESS(status) ((LONG) (status) >= 0)
#endif

#define FORT_STATUS_USER_ERROR STATUS_INVALID_PARAMETER
#define FORT_ERROR_USER_ERROR  ERROR_INVALID_PARAMETER

/* Convert system time to seconds since 1970 */
#define SECSPERDAY 86400
#define SECS_1601_TO_1970                                                                          \
    ((369 * 365 + 89) * (INT64) SECSPERDAY) /* 1601 to 1970 is 369 years plus 89 leap days */
#define fort_system_to_unix_time(system_time) ((system_time) / 10000000 - SECS_1601_TO_1970)

#define FORT_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define FORT_ALIGN_SIZE(size, align) ((((size) + (align - 1)) & ~(align - 1)))

#endif // COMMON_H
