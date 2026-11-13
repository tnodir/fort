#include "dnsmanager.h"

#include <QLoggingCategory>

#include <algorithm>

#include <ares.h>

#include <dns/dnsutil.h>

#include <conf/confmanager.h>
#include <conf/confzonemanager.h>
#include <conf/inioptions.h>
#include <dns/dnsclientmanager.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <util/conf/confbuffer.h>
#include <util/regkey.h>

using namespace Fort;

namespace {

const QLoggingCategory LC("dns.manager");

// The programs connect after the answer and may keep the addresses a bit longer, e.g. of a TTL of 0
inline constexpr quint32 ttlMargin = 60; // seconds

// The Name Resolution Policy Table's rule sends the Dnscache's queries of all names to the listener
inline constexpr char nrptPolicyKey[] =
        R"(SYSTEM\CurrentControlSet\Services\Dnscache\Parameters\DnsPolicyConfig)";
inline constexpr char nrptRuleKey[] = "{B6CDD146-F253-408C-B158-F8D0C0874B12}";

quint32 hostIpsTtl(quint32 ttl)
{
    // The TTL is up to 2^31 - 1 (RFC 2181)
    return qMin(ttl, quint32(INT_MAX)) + ttlMargin;
}

}

DnsManager::DnsManager(QObject *parent) : QObject(parent), m_hostBlocker(m_hostZonesDb)
{
    connect(&m_listener, &DnsListener::queryReceived, this, &DnsManager::processQuery);
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

void DnsManager::setupByConfIni()
{
    const auto &ini = Fort::ini();

    setupBlockZones(ini.dnsBlockZones());

    if (ini.dnsFilterEnabled()) {
        start(ini.dnsServers());
    } else {
        stop();
    }

    Fort::confManager()->setDnsPortsBlocked(isStarted() && ini.dnsBlockPorts());
}

void DnsManager::setupBlockZones(quint32 zonesMask)
{
    if (m_hostBlocker.zonesMask() == zonesMask)
        return;

    m_hostBlocker.setZonesMask(zonesMask);

    // The names' cached answers, found or not, bypass the forwarder
    if (isStarted()) {
        flushResolverCache();
    }
}

// The failed start is retried on the options' change
void DnsManager::start(const QStringList &servers)
{
    if (isStarted() && m_servers == servers)
        return;

    closeForwarder();

    setStatus(openForwarder(servers));
}

void DnsManager::stop()
{
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

    return { .status = nrptAdded ? DnsForwarderStatus::StatusStarted
                                 : DnsForwarderStatus::StatusNrptError };
}

void DnsManager::closeForwarder()
{
    if (!removeNrptRule()) {
        qCWarning(LC) << "NRPT rule removal error";
    }

    m_listener.close();
    closeResolver();

    unloadHosts();
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
        syncZones();
    }

    updateHostRefs();

    flushResolverCache();
}

void DnsManager::unloadHosts()
{
    m_hostZonesDb.close();
    m_hostFiltersMatcher = {};
}

void DnsManager::loadZones()
{
    if (m_hostZonesDb.isOpened() && syncZones()) {
        flushResolverCache();
    }
}

// Returns whether the Zones' names to match changed
bool DnsManager::syncZones()
{
    return taskManager()->taskInfoZoneDownloader()->syncHostZones(m_hostZonesDb);
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

// Returns whether a host() filter or an enabled Hosts Zone was referenced anew
bool DnsManager::updateHostRefs()
{
    const auto &hostRefs = confManager()->hostRefs();

    m_hostFiltersMatcher = hostRefs.hostMatcher();

    const quint32 lastHostId = hostRefs.lastHostId();
    const quint32 refZonesMask = hostRefs.zonesMask();

    // A Zone's enabling is flushed by loadZones()
    const quint32 addedZonesMask = refZonesMask & ~m_refZonesMask & m_hostZonesDb.enabledMask();

    const bool added = (lastHostId != m_lastHostId) || addedZonesMask != 0;

    m_lastHostId = lastHostId;
    m_refZonesMask = refZonesMask;

    return added;
}

void DnsManager::removeZone(quint8 zoneId)
{
    if (m_hostZonesDb.isOpened() && m_hostZonesDb.removeZone(zoneId)) {
        flushResolverCache();
    }
}

// The cached answers bypass the forwarder: their names are resolved anew, adding the IPs
void DnsManager::flushResolverCache()
{
    if (!DnsUtil::flushResolverCache()) {
        qCWarning(LC) << "Flush DNS cache error";
    }
}

HostMatch DnsManager::matchHost(QStringView name) const
{
    HostMatch match = m_hostFiltersMatcher.match(name);
    match.zonesMask = m_hostZonesDb.match(name, confManager()->hostRefs().zonesMask());

    return match;
}

// The answer's addresses are of its last canonical name
HostMatch DnsManager::matchAnswer(const DnsRequest &request, const DnsAnswer &answer) const
{
    HostMatch match = matchHost(request.query.name());

    for (const QString &target : answer.cnameTargets()) {
        match.add(matchHost(target));
    }

    return match;
}

void DnsManager::processQuery(const DnsClient &client, const QByteArray &data)
{
    DnsRequest request = { .client = client };

    if (!request.query.parse(data))
        return;

    const quint8 rcode = checkQuery(request);
    if (rcode != ARES_RCODE_NOERROR) {
        sendRcode(request, rcode);
        return;
    }

    m_resolver->resolve(request);
}

void DnsManager::processAnswer(const DnsRequest &request, const DnsAnswer &answer)
{
    if (m_hostBlocker.isAnswerBlocked(request.query, answer)) {
        sendRcode(request, ARES_RCODE_NXDOMAIN);
        return;
    }

    // The Zones and host() filters may be changed since the query.
    // The client connects to the addresses after the answer.
    const QString &name = request.query.name();
    const DnsAddresses &addresses = answer.addresses();

    if (!writeHostIps(request, answer)) {
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

    if (checkLoopQuery(request))
        return ARES_RCODE_SERVFAIL;

    if (m_hostBlocker.isQueryBlocked(query))
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

bool DnsManager::writeHostIps(const DnsRequest &request, const DnsAnswer &answer)
{
    const DnsAddresses &addresses = answer.addresses();
    if (addresses.isEmpty())
        return true;

    const HostMatch match = matchAnswer(request, answer);
    if (match.isEmpty())
        return true;

    ConfBuffer confBuf;

    confBuf.writeHostIps(
            match, hostIpsTtl(addresses.ttl()), addresses.ip4Array(), addresses.ip6Array());

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
