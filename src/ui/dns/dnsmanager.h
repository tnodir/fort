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

#include "dnshostblocker.h"
#include "hostzonesdb.h"

struct DnsForwarderStatus
{
    enum Status : qint8 {
        StatusOff = 0,
        StatusStarted,
        StatusNrptError, // started, but the system's queries aren't sent to it
        StatusMixedServers, // not started: the plain and DNS over HTTPS servers are mixed
        StatusInvalidServers, // not started: the resolver can't take the servers
        StatusListenError, // not started: the port 53 can't be listened, e.g. it's busy
    };

    bool isError() const { return status >= StatusNrptError; }

    bool operator==(const DnsForwarderStatus &o) const = default;

    Status status = StatusOff;
    QString errorMessage;
};

// The DNS forwarder of the service: resolves the names but the Block Zones' ones, adding the
// addresses of the referenced Hosts Zones' and host() filters' names to them in the driver before
// answering
class DnsManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DnsManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(DnsManager)

    bool isStarted() const { return m_resolver != nullptr; }

    // Of the forwarder by the options, set by the service
    const DnsForwarderStatus &status() const { return m_status; }

    // The rule of a crashed service is removed by the uninstall
    static bool removeNrptRule();

    void setUp() override;
    void tearDown() override;

    // The test of the upstream servers, to start after connecting to its signals
    virtual DnsServersTest *createServersTest(const QStringList &servers, QObject *parent);

signals:
    void statusChanged();

protected:
    void setStatus(const DnsForwarderStatus &status);

private:
    static bool addNrptRule();

    void setupConfManager();
    void setupZones();

    void setupByConfIni();
    void setupBlockZones(quint32 zonesMask);

    void start(const QStringList &servers);
    void stop();

    DnsForwarderStatus openForwarder(const QStringList &servers);
    void closeForwarder();

    DnsForwarderStatus::Status openResolver(const QStringList &servers);
    void closeResolver();

    void loadHosts();
    void unloadHosts();

    void loadZones();
    bool syncZones();
    void loadHostRefs();
    bool updateHostRefs();
    void removeZone(quint8 zoneId);

    void flushResolverCache();

    // The Hosts Zones referenced by the driver's conf and the host() filters listing the name
    HostMatch matchHost(QStringView name) const;

    void processQuery(const DnsClient &client, const QByteArray &data);
    void processAnswer(const DnsRequest &request, const DnsAnswer &answer);

    quint8 checkQuery(const DnsRequest &request);
    bool checkLoopQuery(const DnsRequest &request);
    void logLoop();

    bool writeHostIps(const HostMatch &match, const DnsAddresses &addresses);
    void addConnHostName(const QString &name, const DnsAddresses &addresses);

    void sendRcode(const DnsRequest &request, quint8 rcode);

private:
    bool m_loopLogged = false;

    // The host() filters' last id and the Hosts Zones referenced, as loaded
    quint32 m_lastHostId = 0;
    quint32 m_refZonesMask = 0;

    std::unique_ptr<DnsResolver> m_resolver;

    DnsForwarderStatus m_status;

    QStringList m_servers;

    QList<DnsServersTest *> m_serversTests;

    HostZonesDb m_hostZonesDb;
    DnsHostBlocker m_hostBlocker;
    HostMatcher m_hostFiltersMatcher;

    DnsListener m_listener;

    TriggerTimer m_zonesTimer;
};

#endif // DNSMANAGER_H
