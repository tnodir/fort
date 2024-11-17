#include "confdatarule.h"

#include <common/fortconf.h>

#include <util/net/arearange.h>
#include <util/net/dirrange.h>
#include <util/net/iprange.h>
#include <util/net/portrange.h>
#include <util/net/protorange.h>

ConfDataRule::ConfDataRule(void *data) : ConfData(data) { }

void ConfDataRule::writeRange(const ValueRange *range, qint8 type)
{
    switch (type) {
    case FORT_RULE_FILTER_TYPE_ADDRESS:
    case FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS: {
        writeAddressList(*static_cast<const IpRange *>(range));
    } break;
    case FORT_RULE_FILTER_TYPE_PORT:
    case FORT_RULE_FILTER_TYPE_LOCAL_PORT:
    case FORT_RULE_FILTER_TYPE_PORT_TCP:
    case FORT_RULE_FILTER_TYPE_PORT_UDP: {
        writePortRange(*static_cast<const PortRange *>(range));
    } break;
    case FORT_RULE_FILTER_TYPE_PROTOCOL: {
        writeProtoRange(*static_cast<const ProtoRange *>(range));
    } break;
    case FORT_RULE_FILTER_TYPE_DIRECTION: {
        writeDirRange(*static_cast<const DirRange *>(range));
    } break;
    case FORT_RULE_FILTER_TYPE_AREA: {
        writeAreaRange(*static_cast<const AreaRange *>(range));
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
