#ifndef COMMON_H
#define COMMON_H

#ifndef NT_SUCCESS
#define NT_SUCCESS(status)	((LONG) (status) >= 0)
#endif

#define FORT_STATUS_USER_ERROR	STATUS_INVALID_PARAMETER
#define FORT_ERROR_USER_ERROR	ERROR_INVALID_PARAMETER

/* Convert system time to seconds since 1970 */
#define SECSPERDAY		86400
#define SECS_1601_TO_1970	((369 * 365 + 89) * (INT64) SECSPERDAY)  /* 1601 to 1970 is 369 years plus 89 leap days */
#define fort_system_to_unix_time(system_time)	((system_time) / 10000000 - SECS_1601_TO_1970)

#endif COMMON_H
