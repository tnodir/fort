#include "bitutil.h"

#include <QtAlgorithms>

int BitUtil::bitCount(quint32 v)
{
    return qPopulationCount(v);
}

int BitUtil::firstZeroBit(quint32 v)
{
    return bitScanForward(~v);
}

int BitUtil::bitScanForward(quint32 v)
{
    return qCountTrailingZeroBits(v);
}

int BitUtil::bitScanReverse(quint32 v)
{
    return 31 - int(qCountLeadingZeroBits(v));
}
