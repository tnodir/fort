#ifndef COMMON_H
#define COMMON_H

#if defined(FORT_DRIVER)
#    define NDIS_WDM 1
#    define NDIS630  1

#    if defined(FORT_WIN7_COMPAT)
#        define WIN9X_COMPAT_SPINLOCK /* XXX: Support Windows 7: KeInitializeSpinLock() */
#    endif

#    define POOL_NX_OPTIN 1 /* Enhanced protection of NX pool */

#    define _KRPCENV_ /* To include winerror.h */

#    include <ntddk.h>
#    include <winerror.h>

#    include <fwpmk.h>
#    include <fwpsk.h>
#    include <ntrxdef.h>
#    include <stddef.h>
#else
#    define _WIN32_WINNT 0x0601
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>

#    include <fwpmu.h>
#    include <winioctl.h>
#endif

#if !defined(FORT_API)
#    if defined(FORT_AMALG)
#        define FORT_API static
#    else
#        define FORT_API extern
#    endif
#endif

#if defined(_ARM_) || defined(_ARM64_)
#    define FORT_BIG_ENDIAN 1
#endif

#define UNUSED(p) ((void) (p))

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

#define FORT_BLOCK_REASON_NONE              -1
#define FORT_BLOCK_REASON_UNKNOWN           0
#define FORT_BLOCK_REASON_IP_INET           1
#define FORT_BLOCK_REASON_REAUTH            2
#define FORT_BLOCK_REASON_PROGRAM           3
#define FORT_BLOCK_REASON_APP_GROUP_FOUND   4
#define FORT_BLOCK_REASON_APP_GROUP_DEFAULT 5

#endif // COMMON_H
