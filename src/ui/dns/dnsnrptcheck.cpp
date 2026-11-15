#include "dnsnrptcheck.h"

#include <QHostInfo>
#include <QRandomGenerator>

#include <dns/dnsquery.h>

namespace {

// Never exists (RFC 2606): the upstream servers answer NXDOMAIN, if the query bypasses the listener
inline constexpr char checkNameSuffix[] = ".fort-nrpt-check.invalid";

QString uniqueCheckName()
{
    // Not cached by the system resolver
    const quint64 id = QRandomGenerator::global()->generate64();

    return QString::number(id, 16) + checkNameSuffix;
}

}

DnsNrptCheck::DnsNrptCheck(QObject *parent) : QObject(parent) { }

void DnsNrptCheck::start()
{
    abort();

    m_received = false;
    m_name = uniqueCheckName();

    m_lookupId = QHostInfo::lookupHost(m_name, this, &DnsNrptCheck::onLookedUp);
}

void DnsNrptCheck::abort()
{
    if (m_lookupId < 0)
        return;

    QHostInfo::abortHostLookup(m_lookupId);

    m_lookupId = -1;
    m_name.clear();
}

bool DnsNrptCheck::checkQuery(const DnsQuery &query)
{
    if (m_name.isEmpty() || query.name().compare(m_name, Qt::CaseInsensitive) != 0)
        return false;

    m_received = true;

    return true;
}

void DnsNrptCheck::onLookedUp(const QHostInfo &info)
{
    // Of an aborted lookup
    if (info.lookupId() != m_lookupId)
        return;

    m_lookupId = -1;
    m_name.clear();

    emit finished(m_received);
}
