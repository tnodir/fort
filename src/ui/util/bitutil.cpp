#include "bitutil.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

int BitUtil::firstZeroBit(quint32 u)
{
    const qint32 i = ~u;
    return bitCount((i & (-i)) - 1);
}

int BitUtil::bitCount(quint32 u)
{
#if 0 // TODO: COMPAT: Enable after Win 11 24H2
// #if (defined(_M_IX86) || defined(_M_X64)) && QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    return __popcnt(u);
#elif defined(_M_ARM64)
    __n64 num;
    num.n64_u64[0] = u;

    num = neon_cnt(num);
    return num.n64_u32[0];
#else
    // From http://tekpool.wordpress.com/category/bit-count/
    const quint32 uCount = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
    return ((uCount + (uCount >> 3)) & 030707070707) % 63;

    // The following code is optimized to __popcnt by MSVC 17.9
    /*
    u = u - ((u >> 1) & 0x55555555);
    u = (u & 0x33333333) + ((u >> 2) & 0x33333333);
    return ((u + (u >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
    */
#endif
}

int BitUtil::bitScanForward(quint32 mask)
{
    unsigned long index;
    return _BitScanForward(&index, mask) ? index : -1;
}
