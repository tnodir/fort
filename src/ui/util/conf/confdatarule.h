#ifndef CONFDATARULE_H
#define CONFDATARULE_H

#include "confdata.h"

class AreaRange;
class DirRange;
class PortRange;
class ProtoRange;
class ValueRange;

class ConfDataRule : public ConfData
{
public:
    explicit ConfDataRule(void *data);

    void writeRange(const ValueRange *range, qint8 type);

    static ValueRange *createRangeByType(qint8 type);

private:
    void writePortRange(const PortRange &portRange);
    void writeProtoRange(const ProtoRange &protoRange);
    void writeDirRange(const DirRange &dirRange);
    void writeAreaRange(const AreaRange &areaRange);
};

#endif // CONFDATARULE_H
