#include "dnsclientanswer.h"

#include <QHostAddress>

namespace {

// The records without an address, e.g. "type: 5 alias.com" of a CNAME
inline constexpr char recordTypePrefix[] = "type:";

}

void DnsClientAnswer::readQueryResults(QStringView results)
{
    const auto records = results.split(QLatin1Char(';'), Qt::SkipEmptyParts);

    for (const auto &record : records) {
        const auto text = record.trimmed();

        if (!text.isEmpty() && !text.startsWith(QLatin1String(recordTypePrefix))) {
            readAddress(text);
        }
    }
}

void DnsClientAnswer::readAddress(QStringView text)
{
    const QHostAddress address(text.toString());
    if (address.isNull())
        return;

    // The IPv4 addresses come mapped to IPv6
    bool isIp4 = false;
    const quint32 ip4 = address.toIPv4Address(&isIp4);

    if (isIp4) {
        m_addresses.addIp4(ip4);
        return;
    }

    const Q_IPV6ADDR addr6 = address.toIPv6Address();

    m_addresses.addIp6(addr6.c);
}
