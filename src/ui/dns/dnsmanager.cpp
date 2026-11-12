#include "dnsmanager.h"

#include <QLoggingCategory>

#include <ares.h>

#include <dns/dnsutil.h>

#include <conf/confmanager.h>
#include <conf/confzonemanager.h>
#include <conf/inioptions.h>
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

DnsManager::DnsManager(QObject *parent) : QObject(parent)
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
    stop();
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
    // The removed Zone's id can be reused: its names aren't resolved at once
    connect(confZoneManager, &ConfZoneManager::zoneRemoved, this, &DnsManager::removeZone);
    connect(taskManager, &TaskManager::zonesUpdated, &m_zonesTimer, &TriggerTimer::startTrigger);

    // The removed host() filters' names aren't resolved at once
    connect(confManager, &ConfManager::hostRefsChanged, this, &DnsManager::loadHostRefs);
}

void DnsManager::setupByConfIni()
{
    const auto &ini = Fort::ini();

    if (ini.dnsFilterEnabled()) {
        start(ini.dnsServers());
    } else {
        stop();
    }
}

void DnsManager::start(const QString &servers)
{
    if (isStarted() && m_servers == servers)
        return;

    stop();

    if (!openResolver(servers))
        return;

    if (!m_listener.listen()) {
        closeResolver();
        return;
    }

    m_servers = servers;

    if (!addNrptRule()) {
        qCWarning(LC) << "NRPT rule error";
    }

    loadHosts();
}

void DnsManager::stop()
{
    if (!removeNrptRule()) {
        qCWarning(LC) << "NRPT rule removal error";
    }

    m_listener.close();
    closeResolver();

    unloadHosts();
}

bool DnsManager::openResolver(const QString &servers)
{
    auto resolver = DnsResolver::create(servers);

    if (!resolver->open(servers))
        return false;

    connect(resolver.get(), &DnsResolver::resolved, this, &DnsManager::processAnswer);

    m_resolver = std::move(resolver);

    return true;
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

    m_hostFiltersMatcher = confManager()->hostRefs().hostMatcher();

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

    m_hostFiltersMatcher = confManager()->hostRefs().hostMatcher();

    flushResolverCache();
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
    match.zonesMask = m_hostZonesDb.match(name);

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
    // The Zones and host() filters may be changed since the query.
    // The client connects to the addresses after the answer.
    if (!writeHostIps(matchHost(request.query.name()), answer.addresses())) {
        sendRcode(request, ARES_RCODE_SERVFAIL);
        return;
    }

    m_listener.sendResponse(request.client, answer.response());
}

quint8 DnsManager::checkQuery(const DnsRequest &request) const
{
    const DnsQuery &query = request.query;

    if (query.errorRcode() != ARES_RCODE_NOERROR)
        return query.errorRcode();

    if (isLoopQuery(request))
        return ARES_RCODE_SERVFAIL;

    if (matchHost(query.name()).isEmpty()) {
        qCDebug(LC) << "Blocked:" << query.name();
        return ARES_RCODE_NXDOMAIN;
    }

    return ARES_RCODE_NOERROR;
}

// The upstream servers include the listener
bool DnsManager::isLoopQuery(const DnsRequest &request) const
{
    return m_resolver->isOwnQuery(request.query);
}

bool DnsManager::writeHostIps(const HostMatch &match, const DnsAddresses &addresses)
{
    if (addresses.isEmpty() || match.isEmpty())
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

void DnsManager::sendRcode(const DnsRequest &request, quint8 rcode)
{
    DnsAnswer answer;
    answer.setRcode(request.query, rcode);

    m_listener.sendResponse(request.client, answer.response());
}
