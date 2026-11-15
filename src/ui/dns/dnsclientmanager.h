#ifndef DNSCLIENTMANAGER_H
#define DNSCLIENTMANAGER_H

#include <QDeadlineTimer>
#include <QObject>

#include <dns/dnsetwlistener.h>
#include <util/classhelpers.h>
#include <util/expiringhash.h>
#include <util/ioc/iocservice.h>
#include <util/net/processipkey.h>

// The host names of the processes' addresses by the DNS Client's answers to their queries (the
// cached ones too), for the connections' log: an address keeps its name for a fixed time after the
// last answer, as the answers' events have no TTL. Also the names of the addresses by the service's
// DNS forwarder's answers (to the Dnscache: of any process), by their TTL: they come before the
// connections, while the DNS Client's events usually come after them
class DnsClientManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DnsClientManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(DnsClientManager)

    void setUp() override;
    void tearDown() override;

    struct ConnHostName
    {
        bool ofProcess = false; // by the DNS Client's answer to the process
        QString name;
    };

    // By the DNS Client's answer to the process, else by the DNS forwarder's answer
    ConnHostName connHostName(const ProcessIpKey &key) const;

    void addForwarderAnswer(
            const QString &name, const DnsAddresses &addresses, qint64 lifetimeMsecs);

signals:
    // The address had no or another name: its connections logged before the answer have none or
    // the DNS forwarder's one
    void hostNameAdded(const ProcessIpKey &key, const QString &name);

private:
    void addAnswer(const DnsClientAnswer &answer);
    void addHosts(quint32 pid, const DnsAddresses &addresses, const QString &name,
            const QDeadlineTimer &deadline);
    void addHost(const ProcessIpKey &key, const QString &name, const QDeadlineTimer &deadline);

private:
    ExpiringHash<ProcessIpKey, QString> m_hosts; // the DNS forwarder's ones by the process id 0

    DnsEtwListener m_listener;
};

#endif // DNSCLIENTMANAGER_H
