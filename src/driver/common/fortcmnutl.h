#ifndef FORTCMNUTL_H
#define FORTCMNUTL_H

#include "common.h"

typedef struct fort_string_cmp_arg
{
    PCWSTR s1;
    PCWSTR s2;
    UINT16 n1;
    UINT16 n2;

    UINT16 common_n; /* the common prefix's length */
} FORT_STRING_CMP_ARG, *PFORT_STRING_CMP_ARG;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API int fort_bit_scan_forward(ULONG mask);

FORT_API int fort_mem_cmp(const void *p1, const void *p2, UINT32 len);

#define fort_ip6_cmp(l, r) fort_mem_cmp(l, r, sizeof(ip6_addr_t))

FORT_API BOOL fort_mem_eql(const void *p1, const void *p2, UINT32 len);

/* Compare by UTF-16 code units (as QString) and set the common prefix's length */
FORT_API int fort_string_cmp(PFORT_STRING_CMP_ARG sca);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCMNUTL_H
