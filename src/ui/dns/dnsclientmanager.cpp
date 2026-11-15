#include "dnsclientmanager.h"

namespace {

inline constexpr qint64 hostLifetimeMsecs = 10 * 60 * 1000;
inline constexpr qint64 purgePeriodMsecs = 60 * 1000;

}

DnsClientManager::DnsClientManager(QObject *parent) : QObject(parent), m_hosts(purgePeriodMsecs) { }

void DnsClientManager::setUp()
{
    connect(&m_listener, &DnsEtwListener::answerReceived, this, &DnsClientManager::addAnswer,
            Qt::QueuedConnection);

    // Windows 7 has no answers' events
    m_listener.open();
}

void DnsClientManager::tearDown()
{
    m_listener.close();
}

QString DnsClientManager::hostName(const ProcessIpKey &key) const
{
    return m_hosts.value(key);
}

void DnsClientManager::addAnswer(const DnsClientAnswer &answer)
{
    const quint32 pid = answer.processId();
    const QString &name = answer.name();
    const QDeadlineTimer deadline(hostLifetimeMsecs);

    const DnsAddresses &addresses = answer.addresses();

    for (const quint32 ip4 : addresses.ip4Array()) {
        ip_addr_t ip;
        ip.v4 = ip4;

        addHost(ProcessIpKey(pid, ip, /*isIPv6=*/false), name, deadline);
    }

    for (const ip6_addr_t &ip6 : addresses.ip6Array()) {
        ip_addr_t ip;
        ip.v6 = ip6;

        addHost(ProcessIpKey(pid, ip, /*isIPv6=*/true), name, deadline);
    }
}

void DnsClientManager::addHost(
        const ProcessIpKey &key, const QString &name, const QDeadlineTimer &deadline)
{
    const QString oldName = m_hosts.value(key);

    m_hosts.insert(key, name, deadline);

    if (oldName != name) {
        emit hostNameAdded(key, name);
    }
}
