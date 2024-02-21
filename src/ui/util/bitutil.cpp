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
