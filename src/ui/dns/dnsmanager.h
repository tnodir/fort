#ifndef DNSMANAGER_H
#define DNSMANAGER_H

#include <QObject>

#include <dns/dnslistener.h>
#include <dns/dnsresolver.h>
#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>
#include <util/net/hostmatcher.h>
#include <util/triggertimer.h>

#include "hostzonesdb.h"

// The DNS forwarder of the service: resolves only the names of the Hosts Zones and host() filters,
// adding their addresses to the Zones and host() filters in the driver before answering
class DnsManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DnsManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(DnsManager)

    bool isStarted() const { return m_resolver.isOpened(); }

    // The rule of a crashed service is removed by the uninstall
    static bool removeNrptRule();

    void setUp() override;
    void tearDown() override;

private:
    static bool addNrptRule();

    void setupConfManager();
    void setupZones();

    void setupByConfIni();

    void start(const QString &servers);
    void stop();

    void loadHosts();
    void unloadHosts();

    void loadZones();
    bool syncZones();
    void loadHostRefs();
    void removeZone(quint8 zoneId);

    void flushResolverCache();

    // The Hosts Zones and host() filters listing the name
    HostMatch matchHost(QStringView name) const;

    void processQuery(const DnsClient &client, const QByteArray &data);
    void processAnswer(const DnsRequest &request, const DnsAnswer &answer);

    quint8 checkQuery(const DnsRequest &request) const;
    bool isLoopQuery(const DnsRequest &request) const;

    bool writeHostIps(const HostMatch &match, const DnsAddresses &addresses);

    void sendRcode(const DnsRequest &request, quint8 rcode);

private:
    QString m_servers;

    HostZonesDb m_hostZonesDb;
    HostMatcher m_hostFiltersMatcher;

    DnsListener m_listener;
    DnsResolver m_resolver;

    TriggerTimer m_zonesTimer;
};

#endif // DNSMANAGER_H
