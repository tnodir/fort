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
// last answer, as the answers' events have no TTL
class DnsClientManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DnsClientManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(DnsClientManager)

    void setUp() override;
    void tearDown() override;

    QString hostName(const ProcessIpKey &key) const;

signals:
    // The address had no or another name: its connections logged before the answer have none
    void hostNameAdded(const ProcessIpKey &key, const QString &name);

private:
    void addAnswer(const DnsClientAnswer &answer);
    void addHost(const ProcessIpKey &key, const QString &name, const QDeadlineTimer &deadline);

private:
    ExpiringHash<ProcessIpKey, QString> m_hosts;

    DnsEtwListener m_listener;
};

#endif // DNSCLIENTMANAGER_H
