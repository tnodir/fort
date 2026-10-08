#include "filterlinetext.h"

#include <common/fortconf.h>

#include <util/net/netformatutil.h>
#include <util/net/netutil.h>

#include "conn.h"

namespace {

// By FORT_RULE_FILTER_TYPE_*
const char *const filterNames[] = { "IP", "Port", "Local_IP", "Local_Port", "Proto", "IP_Ver",
    "Dir", "Zones", "Area", "Profile", "Act", "Opt" };

static_assert(std::size(filterNames) == FORT_RULE_FILTER_TYPE_PORT_TCP, "Filter names mismatch");

}

FilterLineText::FilterLineText(const QString &text) : m_text(text) { }

FilterLineText::FilterLineText(const Conn &conn)
{
    addConnAddresses(conn);
    addFilter(FORT_RULE_FILTER_TYPE_ACTION, conn.blocked ? "Allow" : "Block");
}

void FilterLineText::addFilter(qint8 type, const QString &values, bool isNot)
{
    if (values.isEmpty())
        return;

    if (!m_text.isEmpty()) {
        m_text += ':';
    }

    // e.g. "IP(1.1.1.1)", "!Port(80, 443)"
    if (isNot) {
        m_text += '!';
    }

    m_text += QLatin1String(filterNames[type]) + '(' + values + ')';
}

void FilterLineText::addConnAddresses(const Conn &conn)
{
    addFilter(FORT_RULE_FILTER_TYPE_DIRECTION, conn.inbound ? "In" : "Out");
    addFilter(FORT_RULE_FILTER_TYPE_PROTOCOL, NetUtil::protocolName(conn.ipProto));
    addFilter(FORT_RULE_FILTER_TYPE_ADDRESS,
            NetFormatUtil::ipToAddressText(conn.remoteIp, conn.isIPv6));
    addPort(FORT_RULE_FILTER_TYPE_PORT, conn.remotePort);
    addFilter(FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS,
            NetFormatUtil::ipToAddressText(conn.localIp, conn.isIPv6));
    addPort(FORT_RULE_FILTER_TYPE_LOCAL_PORT, conn.localPort);
}

void FilterLineText::addPort(qint8 type, quint16 port)
{
    if (port == 0)
        return; // e.g. ICMP

    addFilter(type, QString::number(port));
}
