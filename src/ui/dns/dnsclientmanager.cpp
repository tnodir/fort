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

QString DnsClientManager::hostName(quint32 pid, const ip_addr_t &ip, bool isIPv6) const
{
    return m_hosts.value(hostKey(pid, ip, isIPv6));
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

        m_hosts.insert(hostKey(pid, ip, /*isIPv6=*/false), name, deadline);
    }

    for (const ip6_addr_t &ip6 : addresses.ip6Array()) {
        ip_addr_t ip;
        ip.v6 = ip6;

        m_hosts.insert(hostKey(pid, ip, /*isIPv6=*/true), name, deadline);
    }
}

DnsClientManager::HostKey DnsClientManager::hostKey(quint32 pid, const ip_addr_t &ip, bool isIPv6)
{
    // Compared and hashed by the bytes: the unused ones are zeroes
    HostKey key;
    std::memset(&key, 0, sizeof(key));

    key.pid = pid;
    key.isIPv6 = isIPv6;

    if (isIPv6) {
        key.ip.v6 = ip.v6;
    } else {
        key.ip.v4 = ip.v4;
    }

    return key;
}
