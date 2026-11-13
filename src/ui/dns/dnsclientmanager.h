#ifndef DNSCLIENTMANAGER_H
#define DNSCLIENTMANAGER_H

#include <QDeadlineTimer>
#include <QHash>
#include <QObject>

#include <cstring>

#include <dns/dnsetwlistener.h>
#include <util/classhelpers.h>
#include <util/expiringhash.h>
#include <util/ioc/iocservice.h>

// The host names of the processes' addresses by the DNS Client's answers to their queries (the
// cached ones too), for the connections' log: an address keeps its name for a fixed time after the
// last answer, as the answers' events have no TTL
class DnsClientManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DnsClientManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(DnsClientManager)

    void setUp() override;
    void tearDown() override;

    QString hostName(quint32 pid, const ip_addr_t &ip, bool isIPv6) const;

private:
    struct HostKey
    {
        quint32 pid;
        quint32 isIPv6;
        ip_addr_t ip;

        friend bool operator==(const HostKey &a, const HostKey &b)
        {
            return std::memcmp(&a, &b, sizeof(HostKey)) == 0;
        }

        friend size_t qHash(const HostKey &key, size_t seed = 0)
        {
            return qHashBits(&key, sizeof(HostKey), seed);
        }
    };

    void addAnswer(const DnsClientAnswer &answer);

    static HostKey hostKey(quint32 pid, const ip_addr_t &ip, bool isIPv6);

private:
    ExpiringHash<HostKey, QString> m_hosts;

    DnsEtwListener m_listener;
};

#endif // DNSCLIENTMANAGER_H
