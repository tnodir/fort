#ifndef VALUERANGEUTIL_H
#define VALUERANGEUTIL_H

#include "valuerange.h"

class ValueRangeUtil
{
public:
    static ValueRange *createRangeByType(qint8 type);
};

#endif // VALUERANGEUTIL_H
