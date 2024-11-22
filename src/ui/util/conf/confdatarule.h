#ifndef CONFDATARULE_H
#define CONFDATARULE_H

#include <util/net/valuerange.h>

class ConfDataRule
{
public:
    static ValueRange *createRangeByType(qint8 type);
};

#endif // CONFDATARULE_H
