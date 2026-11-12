#include "dnshostblocker.h"

#include <QLoggingCategory>

#include <dns/dnsquery.h>

#include "hostzonesdb.h"

namespace {

const QLoggingCategory LC("dns.hostBlocker");

}

DnsHostBlocker::DnsHostBlocker(const HostZonesDb &hostZonesDb) : m_hostZonesDb(hostZonesDb) { }

bool DnsHostBlocker::isQueryBlocked(const DnsQuery &query) const
{
    if (isHostBlocked(query.name())) {
        qCDebug(LC) << "Blocked:" << query.name();
        return true;
    }

    return false;
}

// The Block Zones may be changed while the query waits for the answer
bool DnsHostBlocker::isAnswerBlocked(const DnsQuery &query, const DnsAnswer &answer) const
{
    return isQueryBlocked(query);
}

bool DnsHostBlocker::isHostBlocked(QStringView name) const
{
    return m_hostZonesDb.match(name, m_zonesMask) != 0;
}
