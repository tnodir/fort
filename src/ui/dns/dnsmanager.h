#ifndef DNSMANAGER_H
#define DNSMANAGER_H

#include <QObject>

#include <memory>

#include <dns/dnslistener.h>
#include <dns/dnsresolver.h>
#include <dns/dnsserverstest.h>
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

    bool isStarted() const { return m_resolver != nullptr; }

    // The rule of a crashed service is removed by the uninstall
    static bool removeNrptRule();

    void setUp() override;
    void tearDown() override;

    // The test of the upstream servers, to start after connecting to its signals
    virtual DnsServersTest *createServersTest(const QStringList &servers, QObject *parent);

private:
    static bool addNrptRule();

    void setupConfManager();
    void setupZones();

    void setupByConfIni();

    void start(const QStringList &servers);
    void stop();

    bool openResolver(const QStringList &servers);
    void closeResolver();

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

    quint8 checkQuery(const DnsRequest &request);
    bool checkLoopQuery(const DnsRequest &request);
    void logLoop();

    bool writeHostIps(const HostMatch &match, const DnsAddresses &addresses);

    void sendRcode(const DnsRequest &request, quint8 rcode);

private:
    bool m_loopLogged = false;

    std::unique_ptr<DnsResolver> m_resolver;

    QStringList m_servers;

    HostZonesDb m_hostZonesDb;
    HostMatcher m_hostFiltersMatcher;

    DnsListener m_listener;

    TriggerTimer m_zonesTimer;
};

#endif // DNSMANAGER_H
