#ifndef BITUTIL_H
#define BITUTIL_H

#include <QObject>

class BitUtil
{
public:
    static int firstZeroBit(quint32 u);

    static int bitCount(quint32 u);

    static int bitScanForward(quint32 mask);
};

#endif // BITUTIL_H
