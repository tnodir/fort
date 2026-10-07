#pragma once

#include <googletest.h>

#include <common/fortdef.h>

#include <conf/addressgroup.h>
#include <conf/app.h>
#include <conf/firewallconf.h>
#include <conf/group.h>
#include <conf/rule.h>
#include <driver/drivercommon.h>
#include <manager/envmanager.h>
#include <util/conf/confbuffer.h>
#include <util/net/iprange.h>
#include <util/net/netformatutil.h>

#include "testconfwalkers.h"

namespace {

inline constexpr quint32 connFilterZone1 = (1u << 0);

FORT_CONF_META_CONN connFilterConn(const char *ip, bool inbound = false)
{
    return {
        .inbound = inbound,
        .ip_proto = IpProto_TCP,
        .remote_port = 80,
        .remote_ip = { .v4 = NetFormatUtil::textToIp4(ip) },
    };
}

}

class ConnFilterTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();

protected:
    PFORT_CONF_FLAGS confFlags();

    void setFilterMode(FirewallConf::FilterMode mode);

    void writeConf(quint64 activePeriodsMask = 0);
    void writeRules(const TestRuleList &rules);
    void writeGroups(const QList<Group> &groups, quint32 activeMask);
    void writeZone1(const QString &text);

    DriverCommon::ConnFilterResult connFilter(const QString &appPath, FORT_CONF_META_CONN &conn);
    bool connAllowed(const QString &appPath, FORT_CONF_META_CONN &conn);

protected:
    QList<App> m_apps;

    FirewallConf m_conf;

    ConfBuffer m_confBuf;
    ConfBuffer m_zonesBuf;
    ConfBuffer m_rulesBuf;
    ConfBuffer m_groupsBuf;
};

void ConnFilterTest::SetUp()
{
    m_conf.setupDefaultAddressGroups(); // the Internet excludes the local networks

    m_apps = {
        wildcardApp("C:\\App\\allowed.exe"),
    };

    writeConf();
}

void ConnFilterTest::TearDown() { }

PFORT_CONF_FLAGS ConnFilterTest::confFlags()
{
    return &PFORT_CONF(m_confBuf.data() + DriverCommon::confIoConfOff())->flags;
}

void ConnFilterTest::setFilterMode(FirewallConf::FilterMode mode)
{
    PFORT_CONF_FLAGS flags = confFlags();

    flags->allow_all_new = (mode == FirewallConf::ModeAutoLearn);
    flags->ask_to_connect = (mode == FirewallConf::ModeAskToConnect);
    flags->app_block_all = (mode == FirewallConf::ModeBlockAll);
    flags->app_allow_all = (mode == FirewallConf::ModeAllowAll);
}

void ConnFilterTest::writeConf(quint64 activePeriodsMask)
{
    EnvManager envManager;
    const TestApps apps(m_apps);

    m_conf.resetEdited(FirewallConf::AllEdited);

    if (!m_confBuf.writeConf(m_conf, &apps, &envManager, activePeriodsMask)) {
        qCritical() << "Error:" << m_confBuf.errorMessage();
        Q_UNREACHABLE();
    }
}

void ConnFilterTest::writeRules(const TestRuleList &rules)
{
    if (!m_rulesBuf.writeRules(rules)) {
        qCritical() << "Error:" << m_rulesBuf.errorMessage();
        Q_UNREACHABLE();
    }
}

void ConnFilterTest::writeGroups(const QList<Group> &groups, quint32 activeMask)
{
    m_groupsBuf.writeGroups(TestGroupList(groups), activeMask);
}

void ConnFilterTest::writeZone1(const QString &text)
{
    IpRange ipRange;
    if (!ipRange.fromText(text)) {
        qCritical() << "Error:" << ipRange.errorLineAndMessageDetails();
        Q_UNREACHABLE();
    }

    ConfBuffer zoneBuf;
    zoneBuf.writeZone(ipRange);

    const QList<QByteArray> zonesData = { zoneBuf.buffer() };

    m_zonesBuf.writeZones(connFilterZone1, connFilterZone1, zoneBuf.buffer().size(), zonesData);
}

DriverCommon::ConnFilterResult ConnFilterTest::connFilter(
        const QString &appPath, FORT_CONF_META_CONN &conn)
{
    const char *drvConf = m_confBuf.data() + DriverCommon::confIoConfOff();

    const DriverCommon::ConnFilterConf cf = {
        .drvConfIo = m_confBuf.data(),
        .drvZones = m_zonesBuf.dataOrNull(),
        .drvRules = m_rulesBuf.dataOrNull(),
        .drvGroups = m_groupsBuf.dataOrNull(),
    };

    const FORT_APP_DATA appData = DriverCommon::confAppFind(drvConf, appPath);

    return DriverCommon::confConnFilter(cf, &conn, appData);
}

bool ConnFilterTest::connAllowed(const QString &appPath, FORT_CONF_META_CONN &conn)
{
    return connFilter(appPath, conn) == DriverCommon::ConnFilterAllowed;
}

TEST_F(ConnFilterTest, filterDisabled)
{
    confFlags()->filter_enabled = false;

    FORT_CONF_META_CONN conn = connFilterConn("8.8.8.8");
    ASSERT_TRUE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_UNKNOWN);

    // Collect the traffic
    confFlags()->log_stat_no_filter = true;

    conn = connFilterConn("8.8.8.8");
    ASSERT_TRUE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_UNKNOWN);
}

TEST_F(ConnFilterTest, blockTraffic)
{
    confFlags()->block_traffic = true;

    FORT_CONF_META_CONN conn = connFilterConn("8.8.8.8");
    ASSERT_FALSE(connAllowed("C:\\App\\allowed.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_UNKNOWN);
}

TEST_F(ConnFilterTest, localNetwork)
{
    // Not filtered
    FORT_CONF_META_CONN conn = connFilterConn("192.168.1.1");
    ASSERT_TRUE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_TRUE(conn.is_local_net);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_UNKNOWN);

    // Filtered
    confFlags()->filter_local_net = true;

    conn = connFilterConn("192.168.1.1");
    ASSERT_FALSE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_FILTER_MODE);

    // Block LAN, except the filtered loopback
    confFlags()->filter_locals = true;
    confFlags()->filter_local_net = false;
    confFlags()->block_lan_traffic = true;

    conn = connFilterConn("192.168.1.1");
    ASSERT_FALSE(connAllowed("C:\\App\\allowed.exe", conn));

    conn = connFilterConn("192.168.1.1");
    conn.is_loopback = true;
    ASSERT_TRUE(connAllowed("C:\\App\\unknown.exe", conn));
}

TEST_F(ConnFilterTest, localAddresses)
{
    // Allowed without the Conf
    FORT_CONF_META_CONN conn = connFilterConn("127.0.0.1");
    conn.is_loopback = true;
    ASSERT_TRUE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_UNKNOWN);

    conn = connFilterConn("255.255.255.255");
    ASSERT_TRUE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_TRUE(conn.is_broadcast);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_UNKNOWN);

    // Blocked
    confFlags()->block_traffic = true;

    conn = connFilterConn("127.0.0.1");
    conn.is_loopback = true;
    ASSERT_FALSE(connAllowed("C:\\App\\unknown.exe", conn));

    confFlags()->block_traffic = false;
    confFlags()->block_lan_traffic = true;

    conn = connFilterConn("255.255.255.255");
    ASSERT_FALSE(connAllowed("C:\\App\\unknown.exe", conn));

    // Filtered as the Local Network
    confFlags()->block_lan_traffic = false;
    confFlags()->filter_locals = true;
    confFlags()->filter_local_net = true;

    conn = connFilterConn("127.0.0.1");
    conn.is_loopback = true;
    ASSERT_FALSE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_TRUE(conn.is_local_net);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_FILTER_MODE);
}

TEST_F(ConnFilterTest, blockInetTraffic)
{
    confFlags()->block_inet_traffic = true;

    FORT_CONF_META_CONN conn = connFilterConn("8.8.8.8");
    ASSERT_FALSE(connAllowed("C:\\App\\allowed.exe", conn));
    ASSERT_FALSE(conn.is_local_net);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_UNKNOWN);

    // LAN is not blocked
    conn = connFilterConn("192.168.1.1");
    ASSERT_TRUE(connAllowed("C:\\App\\unknown.exe", conn));

    // The filtered broadcast out of LAN is checked further
    m_conf.inetAddressGroup()->setExcludeText(QString());
    writeConf();

    confFlags()->filter_locals = true;
    confFlags()->block_inet_traffic = true;

    conn = connFilterConn("255.255.255.255");
    ASSERT_TRUE(connAllowed("C:\\App\\allowed.exe", conn));
    ASSERT_FALSE(conn.is_local_net);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);
}

TEST_F(ConnFilterTest, inetAddresses)
{
    m_conf.addressGroups().at(1)->setExcludeText("8.8.8.0/24");
    writeConf();

    FORT_CONF_META_CONN conn = connFilterConn("8.8.8.8");
    ASSERT_FALSE(connAllowed("C:\\App\\allowed.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_IP_INET);

    conn = connFilterConn("8.8.4.4");
    ASSERT_TRUE(connAllowed("C:\\App\\allowed.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);
}

TEST_F(ConnFilterTest, filterModes)
{
    const auto checkMode = [&](FirewallConf::FilterMode mode, FORT_CONF_META_CONN &conn) {
        setFilterMode(mode);

        conn = connFilterConn("8.8.8.8");
        return connFilter("C:\\App\\unknown.exe", conn);
    };

    FORT_CONF_META_CONN conn;

    ASSERT_EQ(checkMode(FirewallConf::ModeAutoLearn, conn), DriverCommon::ConnFilterAllowed);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_FILTER_MODE);

    ASSERT_EQ(checkMode(FirewallConf::ModeAskToConnect, conn), DriverCommon::ConnFilterAsk);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_FILTER_MODE);

    ASSERT_EQ(checkMode(FirewallConf::ModeBlockAll, conn), DriverCommon::ConnFilterBlocked);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_FILTER_MODE);

    ASSERT_EQ(checkMode(FirewallConf::ModeAllowAll, conn), DriverCommon::ConnFilterAllowed);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_FILTER_MODE);

    ASSERT_EQ(checkMode(FirewallConf::ModeIgnore, conn), DriverCommon::ConnFilterIgnored);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_FILTER_MODE);

    // The reused conn's results are reset
    setFilterMode(FirewallConf::ModeBlockAll);

    ASSERT_EQ(connFilter("C:\\App\\unknown.exe", conn), DriverCommon::ConnFilterBlocked);
    ASSERT_FALSE(conn.ignore);

    // The known App
    setFilterMode(FirewallConf::ModeIgnore);

    conn = connFilterConn("8.8.8.8");
    ASSERT_EQ(connFilter("C:\\App\\allowed.exe", conn), DriverCommon::ConnFilterAllowed);
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);
}

TEST_F(ConnFilterTest, appFlags)
{
    App blockedApp = wildcardApp("C:\\App\\blocked.exe");
    blockedApp.blocked = true;

    App inboundApp = wildcardApp("C:\\App\\inbound.exe");
    inboundApp.blockInbound = true;

    App lanApp = wildcardApp("C:\\App\\lan.exe");
    lanApp.lanOnly = true;

    m_apps << blockedApp << inboundApp << lanApp;
    writeConf();

    confFlags()->filter_local_net = true;

    FORT_CONF_META_CONN conn = connFilterConn("8.8.8.8");
    ASSERT_FALSE(connAllowed("C:\\App\\blocked.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);

    // Block Inbound
    conn = connFilterConn("8.8.8.8", /*inbound=*/true);
    ASSERT_FALSE(connAllowed("C:\\App\\inbound.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_BLOCK_INBOUND);

    conn = connFilterConn("8.8.8.8");
    ASSERT_TRUE(connAllowed("C:\\App\\inbound.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);

    // LAN Only
    conn = connFilterConn("8.8.8.8");
    ASSERT_FALSE(connAllowed("C:\\App\\lan.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_LAN_ONLY);

    conn = connFilterConn("192.168.1.1");
    ASSERT_TRUE(connAllowed("C:\\App\\lan.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);
}

TEST_F(ConnFilterTest, groups)
{
    App groupApp = wildcardApp("C:\\App\\group.exe");
    groupApp.groups = 0b011u; // Groups 1 and 2

    m_apps << groupApp;
    writeConf();

    writeRules(TestRuleList({ { .blocked = true, .ruleId = 1, .ruleText = "1.1.1.1:80" } }));

    // The Group 1 is disabled, the Group 2 has the Rule
    writeGroups({ { .groupId = 1 }, { .groupId = 2, .ruleId = 1 } }, /*activeMask=*/0b010u);

    FORT_CONF_META_CONN conn = connFilterConn("8.8.8.8");
    ASSERT_TRUE(connAllowed("C:\\App\\group.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);

    conn = connFilterConn("1.1.1.1");
    ASSERT_FALSE(connAllowed("C:\\App\\group.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_RULE);
    ASSERT_EQ(conn.rule_id, 1);

    // All Groups are disabled
    writeGroups({ { .groupId = 1 }, { .groupId = 2, .ruleId = 1 } }, /*activeMask=*/0);

    conn = connFilterConn("8.8.8.8");
    ASSERT_FALSE(connAllowed("C:\\App\\group.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_GROUP);

    confFlags()->group_blocked = false;

    conn = connFilterConn("8.8.8.8");
    ASSERT_TRUE(connAllowed("C:\\App\\group.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);
}

TEST_F(ConnFilterTest, appRule)
{
    App ruleApp = wildcardApp("C:\\App\\rule.exe");
    ruleApp.ruleId = 2;

    m_apps << ruleApp;
    writeConf();

    writeRules(TestRuleList({
            { .ruleId = 1, .ruleText = "1.1.1.1" },
            { .blocked = true, .ruleId = 2, .ruleText = "2.2.2.2" },
    }));

    FORT_CONF_META_CONN conn = connFilterConn("2.2.2.2");
    ASSERT_FALSE(connAllowed("C:\\App\\rule.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_RULE);
    ASSERT_EQ(conn.rule_id, 2);

    // The reused conn's Rule id is reset
    conn.remote_ip.v4 = NetFormatUtil::textToIp4("1.1.1.1");
    ASSERT_TRUE(connAllowed("C:\\App\\rule.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);
    ASSERT_EQ(conn.rule_id, 0);
}

TEST_F(ConnFilterTest, rulePeriods)
{
    App ruleApp = wildcardApp("C:\\App\\rule.exe");
    ruleApp.ruleId = 1;

    App noPeriodApp = wildcardApp("C:\\App\\no-period.exe");
    noPeriodApp.ruleId = 2;

    m_apps << ruleApp << noPeriodApp;

    writeRules(TestRuleList({
            { .blocked = true, .periodId = 2, .ruleId = 1, .ruleText = "2.2.2.2" },
            { .blocked = true,
                    .periodEnabled = false, // the Time Period isn't applied
                    .periodId = 2,
                    .ruleId = 2,
                    .ruleText = "2.2.2.2" },
    }));

    // The Rules' Time Period is inactive
    writeConf(/*activePeriodsMask=*/0b01);

    FORT_CONF_META_CONN conn = connFilterConn("2.2.2.2");
    ASSERT_TRUE(connAllowed("C:\\App\\rule.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);
    ASSERT_EQ(conn.rule_id, 0);

    conn = connFilterConn("2.2.2.2");
    ASSERT_FALSE(connAllowed("C:\\App\\no-period.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_RULE);
    ASSERT_EQ(conn.rule_id, 2);

    // The Rules' Time Period is active
    writeConf(/*activePeriodsMask=*/0b10);

    conn = connFilterConn("2.2.2.2");
    ASSERT_FALSE(connAllowed("C:\\App\\rule.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_RULE);
    ASSERT_EQ(conn.rule_id, 1);
}

TEST_F(ConnFilterTest, globalRules)
{
    App blockedApp = wildcardApp("C:\\App\\blocked.exe");
    blockedApp.blocked = true;

    m_apps << blockedApp;
    writeConf();

    writeRules(TestRuleList(
            {
                    { .ruleId = 1, .ruleText = "1.1.1.1" },
                    { .blocked = true, .ruleId = 2, .ruleText = "2.2.2.2" },
            },
            /*globPreRuleId=*/1, /*globPostRuleId=*/2));

    // Pre Apps: allow the blocked App
    FORT_CONF_META_CONN conn = connFilterConn("1.1.1.1");
    ASSERT_TRUE(connAllowed("C:\\App\\blocked.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_RULE_GLOB_PRE);
    ASSERT_EQ(conn.rule_id, 1);

    // Post Apps: block the allowed App
    conn = connFilterConn("2.2.2.2");
    ASSERT_FALSE(connAllowed("C:\\App\\allowed.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_RULE_GLOB_POST);
    ASSERT_EQ(conn.rule_id, 2);

    // Post Apps: not checked for the unknown App, blocked by the Filter Mode
    conn = connFilterConn("2.2.2.2");
    ASSERT_FALSE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_FILTER_MODE);
    ASSERT_EQ(conn.rule_id, 0);

    // Post Apps: checked for the unknown App, allowed by the Filter Mode
    setFilterMode(FirewallConf::ModeAllowAll);

    conn = connFilterConn("2.2.2.2");
    ASSERT_FALSE(connAllowed("C:\\App\\unknown.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_RULE_GLOB_POST);
}

TEST_F(ConnFilterTest, appZones)
{
    App acceptApp = wildcardApp("C:\\App\\accept.exe");
    acceptApp.zones.accept_mask = connFilterZone1;

    App rejectApp = wildcardApp("C:\\App\\reject.exe");
    rejectApp.zones.reject_mask = connFilterZone1;

    m_apps << acceptApp << rejectApp;
    writeConf();

    writeZone1("8.0.0.0/8");

    // Accepted Zones
    FORT_CONF_META_CONN conn = connFilterConn("8.8.8.8");
    ASSERT_TRUE(connAllowed("C:\\App\\accept.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_ZONE);
    ASSERT_EQ(conn.act.zone_id, 1);

    conn = connFilterConn("9.9.9.9");
    ASSERT_FALSE(connAllowed("C:\\App\\accept.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_ZONE);

    // Rejected Zones
    conn = connFilterConn("8.8.8.8");
    ASSERT_FALSE(connAllowed("C:\\App\\reject.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_ZONE);
    ASSERT_EQ(conn.act.zone_id, 1);

    conn = connFilterConn("9.9.9.9");
    ASSERT_TRUE(connAllowed("C:\\App\\reject.exe", conn));
    ASSERT_EQ(conn.reason, FORT_CONN_REASON_PROGRAM);
}
