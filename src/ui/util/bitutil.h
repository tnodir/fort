#ifndef BITUTIL_H
#define BITUTIL_H

#include <QObject>

class BitUtil
{
public:
    static int bitCount(quint32 v);

    static int firstZeroBit(quint32 v);

    static int bitScanForward(quint32 v);
};

#endif // BITUTIL_H
