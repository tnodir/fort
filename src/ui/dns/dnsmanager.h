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

#include "dnsnrptcheck.h"
#include "hostzonesdb.h"
#include "hostzonesfill.h"
#include "hostzonessync.h"

struct DnsForwarderStatus
{
    enum Status : qint8 {
        StatusOff = 0,
        StatusStarted,
        StatusChecking, // started, the system's queries aren't known to be sent to it yet
        StatusNrptError, // started, but the system's queries aren't sent to it
        StatusNrptIgnored, // started, but its NRPT rule is overridden or its port is blocked
        StatusMixedServers, // not started: the plain and DNS over HTTPS servers are mixed
        StatusInvalidServers, // not started: the resolver can't take the servers
        StatusListenError, // not started: the port 53 can't be listened, e.g. it's busy
        StatusZonesError, // started, but the Hosts Zones' names can't be opened or written
        StatusBlockZonesError, // started, but no name is resolved: a Block Zone isn't loaded
    };

    bool isError() const { return status >= StatusNrptError; }

    // The system's queries are sent to it
    bool isReceiving() const
    {
        return status == StatusStarted || status == StatusZonesError
                || status == StatusBlockZonesError;
    }

    bool operator==(const DnsForwarderStatus &o) const = default;

    Status status = StatusOff;
    QString errorMessage;
};

// The DNS forwarder of the service: resolves the names but the Block Zones' ones, adding the
// addresses of the referenced Hosts Zones' and host() filters' names to them in the driver before
// answering; an answer's CNAME targets and SVCB/HTTPS TargetNames are matched as its name
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

    // Returns the Block Zones listing the name, else adds the referenced Hosts Zones and host()
    // filters listing it, which the answer's addresses are added to, when it has them
    quint32 matchName(QStringView name, bool matchRefs, HostMatch &match) const;

signals:
    void statusChanged();

protected:
    void setStatus(const DnsForwarderStatus &status);

private:
    static bool addNrptRule();

    void setupConfManager();
    void setupZones();

    void setupByConf();
    void setupByConfIni();
    void setupBlockZones(quint32 zonesMask);

    void start(const QStringList &servers);
    void stop();

    DnsForwarderStatus openForwarder(const QStringList &servers);
    void closeForwarder();

    void startNrptCheck();
    void onNrptChecked(bool received);

    DnsForwarderStatus receivingStatus() const;
    void updateReceivingStatus();
    void setZonesError(const QString &errorMessage);
    void updateBlockZonesFailed(const HostZonesSource &source);

    DnsForwarderStatus::Status openResolver(const QStringList &servers);
    void closeResolver();

    void loadHosts();
    void unloadHosts();

    void openZonesSync();
    void closeZonesSync();

    void loadZones();
    void syncZones();
    void onZonesSynced(bool changed, const QString &errorMessage);
    void finishZonesSync();

    void setQueriesPaused(bool paused);

    void loadHostRefs();
    bool updateHostRefs();
    void removeZone(quint8 zoneId);

    void flushResolverCache();

    bool matchAnswer(const DnsQuery &query, const DnsAnswer &answer, HostMatch &match) const;
    bool matchAnswerName(
            const DnsQuery &query, QStringView name, bool matchRefs, HostMatch &match) const;
    bool isNameBlocked(const DnsQuery &query, QStringView name, quint32 nameZonesMask) const;

    void processQuery(const DnsClient &client, const QByteArray &data);
    void processAnswer(const DnsRequest &request, const DnsAnswer &answer);

    quint8 checkQuery(const DnsRequest &request);
    bool checkLoopQuery(const DnsRequest &request);
    void logLoop();

    bool writeHostIps(const QString &name, const HostMatch &match, const DnsAddresses &addresses);
    void addConnHostName(const QString &name, const DnsAddresses &addresses);

    void sendRcode(const DnsRequest &request, quint8 rcode);

private:
    bool m_loopLogged = false;

    // An enabled Block Zone's names failed to be loaded: the queries fail
    bool m_blockZonesFailed = false;

    bool m_zonesSyncing = false;
    bool m_zonesSyncPending = false;
    bool m_zonesChanged = false; // the names to match, till the syncs' end

    // The host() filters' names added and the Hosts Zones referenced, as loaded
    quint32 m_addedNamesCount = 0;
    quint32 m_refZonesMask = 0;

    quint32 m_blockZonesMask = 0;

    std::unique_ptr<DnsResolver> m_resolver;
    std::unique_ptr<HostZonesSync> m_hostZonesSync;

    DnsForwarderStatus m_status;

    // Of the Hosts Zones' DB's open or of the last sync
    QString m_zonesError;

    QStringList m_servers;

    QList<DnsServersTest *> m_serversTests;

    HostZonesDb m_hostZonesDb;
    HostZonesFill m_hostZonesFill { m_hostZonesDb };
    DnsNrptCheck m_nrptCheck;
    HostMatcher m_hostFiltersMatcher;

    DnsListener m_listener;

    TriggerTimer m_zonesTimer;
};

#endif // DNSMANAGER_H
