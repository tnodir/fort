#include "valuerangeutil.h"

#include <common/fortconf.h>

#include "actionrange.h"
#include "arearange.h"
#include "dirrange.h"
#include "iprange.h"
#include "ipverrange.h"
#include "optionrange.h"
#include "portrange.h"
#include "profilerange.h"
#include "protorange.h"
#include "zonesrange.h"

namespace {

enum RangeType : qint8 {
    RangeTypeIp = 0,
    RangeTypePort,
    RangeTypeProto,
    RangeTypeIpVer,
    RangeTypeDir,
    RangeTypeZones,
    RangeTypeArea,
    RangeTypeProfile,
    RangeTypeAction,
    RangeTypeOption,
};

// Sync with FORT_RULE_FILTER_TYPE enum
RangeType g_filterRangeTypes[] = {
    RangeTypeIp, // FORT_RULE_FILTER_TYPE_ADDRESS = 0,
    RangeTypePort, // FORT_RULE_FILTER_TYPE_PORT,
    RangeTypeIp, // FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS,
    RangeTypePort, // FORT_RULE_FILTER_TYPE_LOCAL_PORT,
    RangeTypeProto, // FORT_RULE_FILTER_TYPE_PROTOCOL,
    RangeTypeIpVer, // FORT_RULE_FILTER_TYPE_IP_VERSION,
    RangeTypeDir, // FORT_RULE_FILTER_TYPE_DIRECTION,
    RangeTypeZones, // FORT_RULE_FILTER_TYPE_ZONES,
    RangeTypeArea, // FORT_RULE_FILTER_TYPE_AREA,
    RangeTypeProfile, // FORT_RULE_FILTER_TYPE_PROFILE,
    RangeTypeAction, // FORT_RULE_FILTER_TYPE_ACTION,
    RangeTypeOption, // FORT_RULE_FILTER_TYPE_OPTION,
    // Complex types
    RangeTypePort, // FORT_RULE_FILTER_TYPE_PORT_TCP,
    RangeTypePort, // FORT_RULE_FILTER_TYPE_PORT_UDP,
};

template<class T>
ValueRange *createRange()
{
    return new T();
}

using createRange_func = ValueRange *(*) (void);

static const createRange_func createRange_funcList[] = {
    &createRange<IpRange>, // RangeTypeIp
    &createRange<PortRange>, // RangeTypePort
    &createRange<ProtoRange>, // RangeTypeProto
    &createRange<IpVerRange>, // RangeTypeIpVer
    &createRange<DirRange>, // RangeTypeDir
    &createRange<ZonesRange>, // RangeZonesZones
    &createRange<AreaRange>, // RangeTypeArea
    &createRange<ProfileRange>, // RangeTypeProfile
    &createRange<ActionRange>, // RangeTypeAction
    &createRange<OptionRange>, // RangeTypeOption
};

}

ValueRange *ValueRangeUtil::createRangeByType(qint8 type)
{
    Q_ASSERT(type >= 0 && type < std::size(g_filterRangeTypes));

    const RangeType rangeType = g_filterRangeTypes[type];

    const createRange_func func = createRange_funcList[rangeType];

    return func();
}
