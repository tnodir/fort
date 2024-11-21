#include "confdatarule.h"

#include <common/fortconf.h>

#include <util/net/arearange.h>
#include <util/net/dirrange.h>
#include <util/net/iprange.h>
#include <util/net/portrange.h>
#include <util/net/profilerange.h>
#include <util/net/protorange.h>

namespace {

enum RangeType : qint8 {
    RangeTypeIp = 0,
    RangeTypePort,
    RangeTypeProto,
    RangeTypeDir,
    RangeTypeArea,
    RangeTypeProfile,
};

// Sync with FORT_RULE_FILTER_TYPE enum
RangeType g_filterRangeTypes[] = {
    RangeTypeIp, // FORT_RULE_FILTER_TYPE_ADDRESS = 0,
    RangeTypePort, // FORT_RULE_FILTER_TYPE_PORT,
    RangeTypeIp, // FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS,
    RangeTypePort, // FORT_RULE_FILTER_TYPE_LOCAL_PORT,
    RangeTypeProto, // FORT_RULE_FILTER_TYPE_PROTOCOL,
    RangeTypeDir, // FORT_RULE_FILTER_TYPE_DIRECTION,
    RangeTypeArea, // FORT_RULE_FILTER_TYPE_AREA,
    RangeTypeProfile, // FORT_RULE_FILTER_TYPE_PROFILE,
    // Complex types
    RangeTypePort, // FORT_RULE_FILTER_TYPE_PORT_TCP,
    RangeTypePort, // FORT_RULE_FILTER_TYPE_PORT_UDP,
};

}

ConfDataRule::ConfDataRule(void *data) : ConfData(data) { }

void ConfDataRule::writeRange(const ValueRange *range, qint8 type)
{
    Q_ASSERT(type >= 0 && type < std::size(g_filterRangeTypes));

    const RangeType rangeType = g_filterRangeTypes[type];

    switch (rangeType) {
    case RangeTypeIp: {
        writeAddressList(*static_cast<const IpRange *>(range));
    } break;
    case RangeTypePort: {
        writePortRange(*static_cast<const PortRange *>(range));
    } break;
    case RangeTypeProto: {
        writeProtoRange(*static_cast<const ProtoRange *>(range));
    } break;
    case RangeTypeDir: {
        writeDirRange(*static_cast<const DirRange *>(range));
    } break;
    case RangeTypeArea: {
        writeAreaRange(*static_cast<const AreaRange *>(range));
    } break;
    case RangeTypeProfile: {
        writeProfileRange(*static_cast<const ProfileRange *>(range));
    } break;
    default:
        Q_UNREACHABLE();
    }
}

void ConfDataRule::writePortRange(const PortRange &portRange)
{
    PFORT_CONF_PORT_LIST portList = PFORT_CONF_PORT_LIST(m_data);

    portList->port_n = quint8(portRange.portSize());
    portList->pair_n = quint8(portRange.pairSize());

    m_data += FORT_CONF_PORT_LIST_OFF;

    writeShorts(portRange.portArray());
    writeShorts(portRange.pairFromArray());
    writeShorts(portRange.pairToArray());
}

void ConfDataRule::writeProtoRange(const ProtoRange &protoRange)
{
    PFORT_CONF_PROTO_LIST protoList = PFORT_CONF_PROTO_LIST(m_data);

    protoList->proto_n = quint8(protoRange.protoSize());
    protoList->pair_n = quint8(protoRange.pairSize());

    m_data += FORT_CONF_PROTO_LIST_OFF;

    writeBytes(protoRange.protoArray());
    writeBytes(protoRange.pairFromArray());
    writeBytes(protoRange.pairToArray());
}

void ConfDataRule::writeDirRange(const DirRange &dirRange)
{
    PFORT_CONF_RULE_FILTER_FLAGS filter = PFORT_CONF_RULE_FILTER_FLAGS(m_data);

    filter->flags = (dirRange.isIn() ? FORT_RULE_FILTER_DIRECTION_IN : 0)
            | (dirRange.isOut() ? FORT_RULE_FILTER_DIRECTION_OUT : 0);

    m_data += sizeof(FORT_CONF_RULE_FILTER_FLAGS);
}

void ConfDataRule::writeAreaRange(const AreaRange &areaRange)
{
    PFORT_CONF_RULE_FILTER_FLAGS filter = PFORT_CONF_RULE_FILTER_FLAGS(m_data);

    filter->flags = (areaRange.isLocalhost() ? FORT_RULE_FILTER_AREA_LOCALHOST : 0)
            | (areaRange.isLan() ? FORT_RULE_FILTER_AREA_LAN : 0)
            | (areaRange.isInet() ? FORT_RULE_FILTER_AREA_INET : 0);

    m_data += sizeof(FORT_CONF_RULE_FILTER_FLAGS);
}

void ConfDataRule::writeProfileRange(const ProfileRange &profileRange)
{
    PFORT_CONF_RULE_FILTER_FLAGS filter = PFORT_CONF_RULE_FILTER_FLAGS(m_data);

    filter->flags = profileRange.profileId();

    m_data += sizeof(FORT_CONF_RULE_FILTER_FLAGS);
}

ValueRange *ConfDataRule::createRangeByType(qint8 type)
{
    Q_ASSERT(type >= 0 && type < std::size(g_filterRangeTypes));

    const RangeType rangeType = g_filterRangeTypes[type];

    switch (rangeType) {
    case RangeTypeIp:
        return new IpRange();
    case RangeTypePort:
        return new PortRange();
    case RangeTypeProto:
        return new ProtoRange();
    case RangeTypeDir:
        return new DirRange();
    case RangeTypeArea:
        return new AreaRange();
    case RangeTypeProfile:
        return new ProfileRange();
    default:
        Q_UNREACHABLE();
        return nullptr;
    }
}
