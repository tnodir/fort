#include "dnsmanager.h"

#include <QLoggingCategory>

#include <algorithm>

#include <ares.h>

#include <dns/dnsutil.h>

#include <conf/confmanager.h>
#include <conf/confzonemanager.h>
#include <conf/firewallconf.h>
#include <conf/inioptions.h>
#include <dns/dnsclientmanager.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <util/conf/confbuffer.h>
#include <util/net/hostrange.h>
#include <util/regkey.h>

using namespace Fort;

namespace {

const QLoggingCategory LC("dns.manager");

// The programs connect after the answer and may keep the addresses a bit longer, e.g. of a TTL of 0
inline constexpr quint32 ttlMargin = 60; // seconds

inline constexpr int nrptRecheckInterval = 60 * 1000; // msecs

// The Name Resolution Policy Table's rule sends the Dnscache's queries of all names to the listener
inline constexpr char nrptPolicyKey[] =
        R"(SYSTEM\CurrentControlSet\Services\Dnscache\Parameters\DnsPolicyConfig)";
inline constexpr char nrptRuleKey[] = "{B6CDD146-F253-408C-B158-F8D0C0874B12}";

quint32 hostIpsTtl(quint32 ttl)
{
    // The TTL is up to 2^31 - 1 (RFC 2181)
    return qMin(ttl, quint32(INT_MAX)) + ttlMargin;
}

HostZonesSource hostZonesSource()
{
    return taskManager()->taskInfoZoneDownloader()->hostZonesSource();
}
}

DnsManager::DnsManager(QObject *parent) : QObject(parent)
{
    connect(&m_listener, &DnsListener::queryReceived, this, &DnsManager::processQuery);
    connect(&m_nrptCheck, &DnsNrptCheck::finished, this, &DnsManager::onNrptChecked);
    connect(&m_zonesTimer, &QTimer::timeout, this, &DnsManager::loadZones);
}

bool DnsManager::addNrptRule()
{
    const RegKey policyReg(RegKey::HKLM, nrptPolicyKey, RegKey::DefaultCreate);
    RegKey reg(policyReg, nrptRuleKey, RegKey::DefaultCreate);

    // ConfigOptions: use the GenericDNSServers
    return reg.setValue("Version", 2) && reg.setValue("Name", QStringList { "." })
            && reg.setValue("GenericDNSServers", "127.0.0.1") && reg.setValue("ConfigOptions", 8)
            && reg.setValue("IPSECCARestriction", QString());
}

bool DnsManager::removeNrptRule()
{
    RegKey policyReg(RegKey::HKLM, nrptPolicyKey, RegKey::DefaultReadWrite);

    const RegKey reg(policyReg, nrptRuleKey);
    if (reg.isNull())
        return true;

    return policyReg.removeKey(nrptRuleKey);
}

void DnsManager::setUp()
{
    closeForwarder(); // the NRPT rule of a crashed service

    setupZones();
    setupConfManager();
}

void DnsManager::tearDown()
{
    closeForwarder();
}

DnsServersTest *DnsManager::createServersTest(const QStringList &servers, QObject *parent)
{
    auto test = new DnsServersTest(servers, parent);

    // Its queries sent to the listener are a loop
    m_serversTests.append(test);

    connect(test, &QObject::destroyed, this, [=, this] { m_serversTests.removeOne(test); });

    return test;
}

void DnsManager::setStatus(const DnsForwarderStatus &status)
{
    if (m_status == status)
        return;

    m_status = status;

    emit statusChanged();
}

void DnsManager::setupConfManager()
{
    auto confManager = Fort::dependency<ConfManager>();

    connect(confManager, &ConfManager::confChanged, this, &DnsManager::setupByConf);
    connect(confManager, &ConfManager::iniChanged, this, &DnsManager::setupByConfIni);

    setupByConfIni();
}

void DnsManager::setupZones()
{
    auto confManager = Fort::dependency<ConfManager>();
    auto confZoneManager = Fort::dependency<ConfZoneManager>();
    auto taskManager = Fort::dependency<TaskManager>();

    m_hostZonesDb.setFilePath(settings()->hostZonesFilePath());

    connect(confZoneManager, &ConfZoneManager::zoneUpdated, &m_zonesTimer,
            &TriggerTimer::startTrigger);
    // The removed Zone's id can be reused: its names aren't matched at once
    connect(confZoneManager, &ConfZoneManager::zoneRemoved, this, &DnsManager::removeZone);
    connect(taskManager, &TaskManager::zonesUpdated, &m_zonesTimer, &TriggerTimer::startTrigger);

    connect(confManager, &ConfManager::hostRefsChanged, this, &DnsManager::loadHostRefs);
}

void DnsManager::setupByConf()
{
    if (Fort::conf().dnsForwarderEnabled()) {
        start(Fort::ini().dnsServers());
    } else {
        stop();
    }
}

void DnsManager::setupByConfIni()
{
    setupBlockZones(Fort::ini().dnsBlockZones());

    setupByConf();
}

void DnsManager::setupBlockZones(quint32 zonesMask)
{
    if (m_blockZonesMask == zonesMask)
        return;

    m_blockZonesMask = zonesMask;

    if (!isStarted())
        return;

    updateBlockZonesState(hostZonesSource());

    // The names' cached answers, found or not, bypass the forwarder
    flushResolverCache();
}

// The failed start is retried on the options' change
void DnsManager::start(const QStringList &servers)
{
    if (isStarted() && m_servers == servers)
        return;

    closeForwarder();

    setStatus(openForwarder(servers));

    startNrptCheck();
}

void DnsManager::stop()
{
    if (m_status.status == DnsForwarderStatus::StatusOff)
        return;

    closeForwarder();

    setStatus({});
}

DnsForwarderStatus DnsManager::openForwarder(const QStringList &servers)
{
    const DnsForwarderStatus::Status status = openResolver(servers);
    if (status != DnsForwarderStatus::StatusStarted) {
        return { .status = status };
    }

    if (!m_listener.listen()) {
        closeResolver();
        return {
            .status = DnsForwarderStatus::StatusListenError,
            .errorMessage = m_listener.errorString(),
        };
    }

    m_servers = servers;
    m_loopLogged = false;

    const bool nrptAdded = addNrptRule();
    if (!nrptAdded) {
        qCWarning(LC) << "NRPT rule error";
    }

    loadHosts();

    if (!nrptAdded) {
        return { .status = DnsForwarderStatus::StatusNrptError };
    }

    return { .status = DnsForwarderStatus::StatusChecking };
}

void DnsManager::closeForwarder()
{
    if (!removeNrptRule()) {
        qCWarning(LC) << "NRPT rule removal error";
    }

    m_nrptCheck.abort();

    m_listener.close();
    closeResolver();

    unloadHosts();
}

// Its query would wait in the paused listener
void DnsManager::startNrptCheck()
{
    if (m_status.status != DnsForwarderStatus::StatusChecking || m_listener.isPaused())
        return;

    m_nrptCheck.start();
}

// A query received may be the system's one: e.g. the rule applied or the blocking filter removed
// after the check
void DnsManager::recheckNrptIgnored()
{
    if (m_status.status != DnsForwarderStatus::StatusNrptIgnored)
        return;

    if (m_nrptCheck.isRunning() || m_nrptCheck.isStartedWithin(nrptRecheckInterval))
        return;

    m_nrptCheck.start();
}

void DnsManager::onNrptChecked(bool received)
{
    const bool wasIgnored = (m_status.status == DnsForwarderStatus::StatusNrptIgnored);

    if (received) {
        if (wasIgnored) {
            qCInfo(LC) << "NRPT rule: the system's queries are received after the check failed";
        }

        setStatus(receivingStatus());
    } else if (!wasIgnored) {
        qCWarning(LC) << "NRPT rule ignored: the system's queries aren't received";

        setStatus({ .status = DnsForwarderStatus::StatusNrptIgnored });
    }
}

DnsForwarderStatus DnsManager::receivingStatus() const
{
    if (m_blockZonesFailed) {
        return {
            .status = DnsForwarderStatus::StatusBlockZonesError,
            .errorMessage = m_zonesError,
        };
    }

    if (!m_zonesError.isEmpty()) {
        return { .status = DnsForwarderStatus::StatusZonesError, .errorMessage = m_zonesError };
    }

    if (m_blockZonesNotUpdated) {
        return { .status = DnsForwarderStatus::StatusBlockZonesNotUpdated };
    }

    return { .status = DnsForwarderStatus::StatusStarted };
}

// Shown while the system's queries are received: else their error is shown
void DnsManager::updateReceivingStatus()
{
    if (m_status.isReceiving()) {
        setStatus(receivingStatus());
    }
}

void DnsManager::setZonesError(const QString &errorMessage)
{
    m_zonesError = errorMessage;

    updateReceivingStatus();
}

// An enabled Block Zone having its cache file, but no names in the DB, after the DB's open or the
// sync failed (e.g. the cache file being rewritten by its download); the sync's end updates it.
// Having neither: not updated yet or its download failed.
void DnsManager::updateBlockZonesState(const HostZonesSource &source)
{
    if (m_zonesSyncing)
        return;

    const quint32 zonesMask = source.enabledMask & m_blockZonesMask;
    const bool failed = m_hostZonesFill.hasZonesToFill(source, zonesMask);
    const bool notUpdated = m_hostZonesFill.hasZonesUncached(source, zonesMask);

    if (m_blockZonesFailed == failed && m_blockZonesNotUpdated == notUpdated)
        return;

    if (failed && !m_blockZonesFailed) {
        qCWarning(LC) << "Block Zones' names aren't loaded: the queries fail";
    }

    if (notUpdated && !m_blockZonesNotUpdated) {
        qCWarning(LC) << "Block Zones aren't updated: their names are resolved";
    }

    m_blockZonesFailed = failed;
    m_blockZonesNotUpdated = notUpdated;

    updateReceivingStatus();
}

DnsForwarderStatus::Status DnsManager::openResolver(const QStringList &servers)
{
    auto resolver = DnsResolver::create(servers);
    if (!resolver) {
        qCWarning(LC) << "Mixed plain and DNS over HTTPS servers:" << servers;
        return DnsForwarderStatus::StatusMixedServers;
    }

    if (!resolver->open(servers))
        return DnsForwarderStatus::StatusInvalidServers;

    connect(resolver.get(), &DnsResolver::resolved, this, &DnsManager::processAnswer);

    m_resolver = std::move(resolver);

    return DnsForwarderStatus::StatusStarted;
}

void DnsManager::closeResolver()
{
    m_resolver.reset();
}

void DnsManager::loadHosts()
{
    if (m_hostZonesDb.open()) {
        openZonesSync();
        syncZones();
    } else {
        m_zonesError = m_hostZonesDb.errorMessage();

        updateBlockZonesState(hostZonesSource());
    }

    updateHostRefs();

    flushResolverCache();
}

void DnsManager::unloadHosts()
{
    closeZonesSync();

    m_hostZonesDb.close();
    m_hostZonesFill.clear();
    m_hostFiltersMatcher = {};

    m_zonesError.clear();
    m_blockZonesFailed = false;
    m_blockZonesNotUpdated = false;
}

void DnsManager::openZonesSync()
{
    m_hostZonesSync = std::make_unique<HostZonesSync>();

    // The result reported before the sync's close isn't received
    connect(m_hostZonesSync.get(), &HostZonesSync::synced, m_hostZonesSync.get(),
            [this](bool changed, const QString &errorMessage) {
                onZonesSynced(changed, errorMessage);
            });
}

// Waits for the Zone's transaction being written
void DnsManager::closeZonesSync()
{
    m_hostZonesSync.reset();

    m_zonesSyncing = false;
    m_zonesSyncPending = false;
    m_zonesChanged = false;
}

// The DB failed to open isn't opened again till the forwarder's restart
void DnsManager::loadZones()
{
    if (m_hostZonesDb.isOpened()) {
        syncZones();
    } else {
        updateBlockZonesState(hostZonesSource());
    }
}

// The worker writes the names by its own connection: the matches meanwhile read the last commit
void DnsManager::syncZones()
{
    const HostZonesSource source = hostZonesSource();

    // A Zone's disabling at once, its old names till the commit of the new ones
    m_zonesChanged |= m_hostZonesFill.update(source);

    // The names of a Zone filled anew aren't matched till the commit: its queries wait for it
    const bool fillZones = m_hostZonesFill.hasZonesToFill(source, source.enabledMask);

    if (m_zonesSyncing) {
        m_zonesSyncPending = true;

        if (fillZones) {
            setQueriesPaused(true);
        }
        return;
    }

    m_zonesSyncing = true;

    setQueriesPaused(fillZones);

    m_hostZonesSync->sync(m_hostZonesDb.filePath(), source);
}

// A Zone failed to be written keeps its old names and is written again by a later sync
void DnsManager::onZonesSynced(bool changed, const QString &errorMessage)
{
    m_zonesSyncing = false;
    m_zonesChanged |= changed;

    setZonesError(errorMessage);

    m_hostZonesDb.loadPatterns();

    if (m_zonesSyncPending) {
        m_zonesSyncPending = false;
        syncZones();
        return;
    }

    finishZonesSync();
}

void DnsManager::finishZonesSync()
{
    const HostZonesSource source = hostZonesSource();

    m_zonesChanged |= m_hostZonesFill.update(source);
    updateBlockZonesState(source);

    setQueriesPaused(false);

    if (m_zonesChanged) {
        m_zonesChanged = false;
        flushResolverCache();
    }
}

// The queries wait in the listener's sockets
void DnsManager::setQueriesPaused(bool paused)
{
    if (m_listener.isPaused() == paused)
        return;

    m_listener.setPaused(paused);

    if (paused) {
        m_nrptCheck.abort();
    } else {
        startNrptCheck();
    }
}

void DnsManager::loadHostRefs()
{
    if (!isStarted())
        return;

    // The removed references' names don't add their IPs at once, the new ones' names are resolved
    // anew
    if (updateHostRefs()) {
        flushResolverCache();
    }
}

// Returns whether a host() filter's name or an enabled Hosts Zone was referenced anew
bool DnsManager::updateHostRefs()
{
    const auto &hostRefs = confManager()->hostRefs();

    m_hostFiltersMatcher = hostRefs.hostMatcher();

    const quint32 addedNamesCount = hostRefs.addedNamesCount();
    const quint32 refZonesMask = hostRefs.zonesMask();

    // A Zone's enabling is flushed by loadZones()
    const quint32 addedZonesMask = refZonesMask & ~m_refZonesMask & m_hostZonesDb.enabledMask();

    const bool added = (addedNamesCount != m_addedNamesCount) || addedZonesMask != 0;

    m_addedNamesCount = addedNamesCount;
    m_refZonesMask = refZonesMask;

    return added;
}

void DnsManager::removeZone(quint8 zoneId)
{
    if (!m_hostZonesDb.isOpened())
        return;

    m_hostZonesFill.removeZone(zoneId);

    m_zonesTimer.startTrigger();
}

// The cached answers bypass the forwarder: their names are resolved anew, adding the IPs
void DnsManager::flushResolverCache()
{
    if (!DnsUtil::flushResolverCache()) {
        qCWarning(LC) << "Flush DNS cache error";
    }
}

// Returns whether a Block Zone lists the query's name or a target, else adds the referenced Hosts
// Zones and host() filters listing them. The answer's addresses are of its last canonical name, its
// hints of the SVCB/HTTPS records' targets
bool DnsManager::matchAnswer(const DnsQuery &query, const DnsAnswer &answer, HostMatch &match) const
{
    // The referenced names add the answer's addresses
    const bool matchRefs = !answer.addresses().isEmpty();

    if (matchAnswerName(query, query.name(), matchRefs, match))
        return true;

    for (const QString &target : answer.targetNames()) {
        if (matchAnswerName(query, target, matchRefs, match))
            return true;
    }

    return false;
}

bool DnsManager::matchAnswerName(
        const DnsQuery &query, QStringView name, bool matchRefs, HostMatch &match) const
{
    return isNameBlocked(query, name, matchName(name, matchRefs, match));
}

// One lookup of the name by the Block and referenced Zones
quint32 DnsManager::matchName(QStringView name, bool matchRefs, HostMatch &match) const
{
    const QByteArray key = HostRange::normalizedKey(name);

    const quint32 refZonesMask = matchRefs ? m_refZonesMask : 0;
    const quint32 zonesMask = m_hostZonesDb.matchKey(key, m_blockZonesMask | refZonesMask);

    const quint32 blockZonesMask = zonesMask & m_blockZonesMask;
    if (blockZonesMask != 0)
        return blockZonesMask;

    if (matchRefs) {
        match.zonesMask |= zonesMask & refZonesMask;
        match.add(m_hostFiltersMatcher.matchKey(key));
    }

    return 0;
}

// The names of the Block Zones aren't found, even if a host() filter lists them. The Block Zones
// may be changed while the query waits for the answer. Their names may be the canonical names or
// the SVCB/HTTPS records' targets of another name's answer, e.g. of a tracker behind the site's
// subdomain
bool DnsManager::isNameBlocked(const DnsQuery &query, QStringView name, quint32 nameZonesMask) const
{
    if ((nameZonesMask & m_blockZonesMask) == 0)
        return false;

    if (name == query.name()) {
        qCDebug(LC) << "Blocked:" << name;
    } else {
        qCDebug(LC) << "Blocked:" << query.name() << "Target:" << name;
    }

    return true;
}

void DnsManager::processQuery(const DnsClient &client, const QByteArray &data)
{
    DnsRequest request = { .client = client };

    if (!request.query.parse(data))
        return;

    recheckNrptIgnored();

    const quint8 rcode = checkQuery(request);
    if (rcode != ARES_RCODE_NOERROR) {
        sendRcode(request, rcode);
        return;
    }

    m_resolver->resolve(request);
}

void DnsManager::processAnswer(const DnsRequest &request, const DnsAnswer &answer)
{
    if (m_blockZonesFailed) {
        sendRcode(request, ARES_RCODE_SERVFAIL);
        return;
    }

    // The Zones and host() filters may be changed since the query.
    HostMatch match;
    if (matchAnswer(request.query, answer, match)) {
        sendRcode(request, ARES_RCODE_NXDOMAIN);
        return;
    }

    // The client connects to the addresses after the answer.
    const QString &name = request.query.name();
    const DnsAddresses &addresses = answer.addresses();

    if (!writeHostIps(name, match, addresses)) {
        sendRcode(request, ARES_RCODE_SERVFAIL);
        return;
    }

    addConnHostName(name, addresses);

    m_listener.sendResponse(request.client, answer.response());
}

quint8 DnsManager::checkQuery(const DnsRequest &request)
{
    const DnsQuery &query = request.query;

    if (query.errorRcode() != ARES_RCODE_NOERROR)
        return query.errorRcode();

    if (m_nrptCheck.checkQuery(query))
        return ARES_RCODE_NXDOMAIN;

    if (checkLoopQuery(request))
        return ARES_RCODE_SERVFAIL;

    // The names to block aren't known
    if (m_blockZonesFailed)
        return ARES_RCODE_SERVFAIL;

    const QString &name = query.name();
    if (isNameBlocked(query, name, m_hostZonesDb.match(name, m_blockZonesMask)))
        return ARES_RCODE_NXDOMAIN;

    return ARES_RCODE_NOERROR;
}

// The upstream servers of the resolver or of a servers' test include the listener
bool DnsManager::checkLoopQuery(const DnsRequest &request)
{
    const DnsQuery &query = request.query;

    if (m_resolver->isOwnQuery(query)) {
        logLoop();
        return true;
    }

    return std::any_of(m_serversTests.begin(), m_serversTests.end(),
            [&](DnsServersTest *test) { return test->checkLoopQuery(query); });
}

// Once per the forwarder's start: each query loops
void DnsManager::logLoop()
{
    if (m_loopLogged)
        return;

    m_loopLogged = true;

    qCWarning(LC) << "Loop: the upstream servers include the forwarder:" << m_servers;
}

// The match is empty for the answer without addresses
bool DnsManager::writeHostIps(
        const QString &name, const HostMatch &match, const DnsAddresses &addresses)
{
    if (match.isEmpty())
        return true;

    ConfBuffer confBuf;

    if (!confBuf.writeHostIps(
                match, hostIpsTtl(addresses.ttl()), addresses.ip4Array(), addresses.ip6Array())) {
        qCWarning(LC) << "Host IPs error:" << name << confBuf.errorMessage();
        return false;
    }

    auto driverManager = Fort::driverManager();

    if (!driverManager->writeHostIps(confBuf.buffer())) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    return true;
}

void DnsManager::addConnHostName(const QString &name, const DnsAddresses &addresses)
{
    if (addresses.isEmpty())
        return;

    const qint64 lifetimeMsecs = qint64(hostIpsTtl(addresses.ttl())) * 1000;

    Fort::dnsClientManager()->addForwarderAnswer(name, addresses, lifetimeMsecs);
}

void DnsManager::sendRcode(const DnsRequest &request, quint8 rcode)
{
    DnsAnswer answer;
    answer.setRcode(request.query, rcode);

    m_listener.sendResponse(request.client, answer.response());
}
