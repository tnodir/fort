/* Fort Firewall Common Utilities */

#include "fortcmnutl.h"

FORT_API int fort_bit_scan_forward(ULONG mask)
{
    unsigned long index;
    return _BitScanForward(&index, mask) ? index : -1;
}

FORT_API int fort_mem_cmp(const void *p1, const void *p2, UINT32 len)
{
    const size_t n = RtlCompareMemory(p1, p2, len);
    return (n == len) ? 0 : (((unsigned char *) p1)[n] - ((unsigned char *) p2)[n]);
}

FORT_API BOOL fort_mem_eql(const void *p1, const void *p2, UINT32 len)
{
    return RtlCompareMemory(p1, p2, len) == len;
}

FORT_API int fort_string_cmp(PFORT_STRING_CMP_ARG sca)
{
    PCWSTR s1 = sca->s1;
    PCWSTR s2 = sca->s2;
    const UINT16 min_n = (sca->n1 < sca->n2) ? sca->n1 : sca->n2;

    UINT16 n = 0;
    while (n < min_n && s1[n] == s2[n]) {
        ++n;
    }

    sca->common_n = n;

    if (n < min_n) {
        return (int) s1[n] - (int) s2[n];
    }

    return (int) sca->n1 - (int) sca->n2;
}
