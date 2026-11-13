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

DnsClientManager::ConnHostName DnsClientManager::connHostName(const ProcessIpKey &key) const
{
    const QString name = m_hosts.value(key);
    if (!name.isEmpty()) {
        return { .ofProcess = true, .name = name };
    }

    return { .name = m_hosts.value(ProcessIpKey(/*pid=*/0, key.ip, key.isIPv6)) };
}

void DnsClientManager::addForwarderAnswer(
        const QString &name, const DnsAddresses &addresses, qint64 lifetimeMsecs)
{
    addHosts(/*pid=*/0, addresses, name, QDeadlineTimer(lifetimeMsecs));
}

void DnsClientManager::addAnswer(const DnsClientAnswer &answer)
{
    addHosts(answer.processId(), answer.addresses(), answer.name(),
            QDeadlineTimer(hostLifetimeMsecs));
}

void DnsClientManager::addHosts(quint32 pid, const DnsAddresses &addresses, const QString &name,
        const QDeadlineTimer &deadline)
{
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

    // The forwarder's names don't change the logged ones
    if (key.pid != 0 && oldName != name) {
        emit hostNameAdded(key, name);
    }
}
