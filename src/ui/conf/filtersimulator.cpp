#include "filtersimulator.h"

#include <dns/dnsmanager.h>
#include <fortglobal.h>
#include <manager/envmanager.h>
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <util/bitutil.h>

#include "confappmanager.h"
#include "confgroupmanager.h"
#include "confmanager.h"
#include "confrulemanager.h"

using namespace Fort;

namespace {

bool connDecisionChanged(const FORT_CONF_META_CONN &conn, const FORT_CONF_META_CONN &other)
{
    if (conn.reason != other.reason || conn.rule_id != other.rule_id)
        return true;

    return conn.act.zone_id != other.act.zone_id || conn.act.terminated != other.act.terminated;
}

// The DNS Forwarder while it receives the system's queries, whose answers add their addresses to
// the driver; else null
DnsManager *receivingDnsManager()
{
    auto dnsManager = Fort::dnsManager();
    if (dnsManager == nullptr || !dnsManager->status().isReceiving())
        return nullptr;

    return dnsManager;
}

// The DNS Forwarder's match of the host name, as of an answer with addresses
FilterSimConn::HostNameResult matchSimHostName(
        const QString &hostName, HostMatch &match, quint8 &blockZoneId)
{
    auto dnsManager = receivingDnsManager();
    if (dnsManager == nullptr)
        return FilterSimConn::HostNameNoForwarder;

    const quint32 blockZonesMask = dnsManager->matchName(hostName, /*matchRefs=*/true, match);
    if (blockZonesMask != 0) {
        blockZoneId = BitUtil::bitScanForward(blockZonesMask) + 1;
        return FilterSimConn::HostNameBlocked;
    }

    return FilterSimConn::HostNameMatched;
}

// The remote IP is of the host name's addresses, which the DNS Forwarder adds to the driver
void simulateConnHostName(DriverCommon::ConnFilterConf cf, FilterSimConn &simConn,
        const FORT_CONF_APP_FIND_RESULT &app)
{
    HostMatch match;
    quint8 blockZoneId = 0;

    simConn.hostNameResult = matchSimHostName(simConn.hostName, match, blockZoneId);

    FORT_CONF_META_CONN &conn = simConn.conn;

    if (simConn.hostNameResult == FilterSimConn::HostNameBlocked) {
        // The name isn't resolved: the program doesn't connect
        simConn.result = DriverCommon::ConnFilterBlocked;
        conn.app = app;
        conn.act.zone_id = blockZoneId;
        return;
    }

    conn.host_zones_mask = match.zonesMask;

    cf.hostNameIds = &match.nameIds;

    simConn.result = DriverCommon::confConnFilter(cf, &conn, app);
}

// The remote IP's names aren't known
void simulateConnUnknownHost(const DriverCommon::ConnFilterConf &cf, FilterSimConn &simConn,
        const FORT_CONF_APP_FIND_RESULT &app, const HostMatch &allHostMatch)
{
    simConn.result = DriverCommon::confConnFilter(cf, &simConn.conn, app);

    simConn.hostDependent = FilterSimulator::isConnHostDependent(cf, simConn, app, allHostMatch);
}

}

bool FilterSimulator::writeConf()
{
    auto confManager = Fort::confManager();

    if (!m_confBuf.writeConf(confManager->conf(), confAppManager(), envManager(),
                confManager->driverConfState())) {
        m_errorMessage = "Conf error: " + m_confBuf.errorMessage();
        return false;
    }

    const auto zones = taskManager()->taskInfoZoneDownloader()->loadZonesData();

    m_zonesBuf.writeZones(zones.dataZonesMask, zones.enabledMask, zones.dataSize, zones.zonesData);

    if (receivingDnsManager() != nullptr) {
        m_allHostMatch = { .zonesMask = zones.hostsMask,
            .nameIds = confManager->hostRefs().nameIds() };
    }

    if (!writeRules())
        return false;

    m_groupsBuf.writeGroups(*confGroupManager(), confGroupManager()->activeGroupsMask());

    return true;
}

bool FilterSimulator::writeRules()
{
    m_rulesBuf.writeRules(*confRuleManager());

    if (m_rulesBuf.hasError()) {
        m_errorMessage = "Rules error: " + m_rulesBuf.errorMessage();
        return false;
    }

    return true;
}

void FilterSimulator::simulate(FilterSimConn &simConn) const
{
    const char *drvConf = m_confBuf.data() + DriverCommon::confIoConfOff();

    const DriverCommon::ConnFilterConf cf = connFilterConf();
    const FORT_CONF_APP_FIND_RESULT app = DriverCommon::confAppFind(drvConf, simConn.appPath);

    if (!simConn.hostName.isEmpty()) {
        simulateConnHostName(cf, simConn, app);
    } else {
        simulateConnUnknownHost(cf, simConn, app, m_allHostMatch);
    }

    simConn.conn.app.rule = nullptr; // points into the conf buffer
}

bool FilterSimulator::isConnHostDependent(DriverCommon::ConnFilterConf cf,
        const FilterSimConn &simConn, const FORT_CONF_APP_FIND_RESULT &app,
        const HostMatch &allHostMatch)
{
    if (allHostMatch.isEmpty())
        return false;

    FORT_CONF_META_CONN conn = simConn.conn;
    conn.host_zones_mask = allHostMatch.zonesMask;

    cf.hostNameIds = &allHostMatch.nameIds;

    const DriverCommon::ConnFilterResult result = DriverCommon::confConnFilter(cf, &conn, app);

    return result != simConn.result || connDecisionChanged(conn, simConn.conn);
}

DriverCommon::ConnFilterConf FilterSimulator::connFilterConf() const
{
    return {
        .drvConfIo = m_confBuf.data(),
        .drvZones = m_zonesBuf.dataOrNull(),
        .drvRules = m_rulesBuf.dataOrNull(),
        .drvGroups = m_groupsBuf.dataOrNull(),
    };
}
