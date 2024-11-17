#include "confdatarule.h"

#include <common/fortconf.h>

#include <util/net/arearange.h>
#include <util/net/dirrange.h>
#include <util/net/iprange.h>
#include <util/net/portrange.h>
#include <util/net/protorange.h>

ConfDataRule::ConfDataRule(void *data) : ConfData(data) { }

ValueRange *ConfDataRule::createRangeByType(qint8 type)
{
    switch (type) {
    case FORT_RULE_FILTER_TYPE_ADDRESS:
    case FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS:
        return new IpRange();
    case FORT_RULE_FILTER_TYPE_PORT:
    case FORT_RULE_FILTER_TYPE_LOCAL_PORT:
    case FORT_RULE_FILTER_TYPE_PORT_TCP:
    case FORT_RULE_FILTER_TYPE_PORT_UDP:
        return new PortRange();
    case FORT_RULE_FILTER_TYPE_PROTOCOL:
        return new ProtoRange();
    case FORT_RULE_FILTER_TYPE_DIRECTION:
        return new DirRange();
    case FORT_RULE_FILTER_TYPE_AREA:
        return new AreaRange();
    default:
        Q_UNREACHABLE();
        return nullptr;
    }
}
