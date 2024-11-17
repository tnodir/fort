#ifndef CONFDATARULE_H
#define CONFDATARULE_H

#include "confdata.h"

class ValueRange;

class ConfDataRule : public ConfData
{
public:
    explicit ConfDataRule(void *data);

    static ValueRange *createRangeByType(qint8 type);
};

#endif // CONFDATARULE_H
