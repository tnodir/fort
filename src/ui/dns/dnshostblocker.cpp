#include "dnshostblocker.h"

#include <QLoggingCategory>

#include <dns/dnsanswer.h>
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

// The Block Zones may be changed while the query waits for the answer. Their names may be the
// canonical names of another name's answer, e.g. of a tracker behind the site's subdomain
bool DnsHostBlocker::isAnswerBlocked(const DnsQuery &query, const DnsAnswer &answer) const
{
    if (isQueryBlocked(query))
        return true;

    for (const QString &target : answer.cnameTargets()) {
        if (isHostBlocked(target)) {
            qCDebug(LC) << "Blocked:" << query.name() << "CNAME:" << target;
            return true;
        }
    }

    return false;
}

bool DnsHostBlocker::isHostBlocked(QStringView name) const
{
    return m_hostZonesDb.match(name, m_zonesMask) != 0;
}
