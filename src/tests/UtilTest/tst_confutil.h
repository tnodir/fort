#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <conf/addressgroup.h>
#include <conf/appgroup.h>
#include <conf/confrulemanager.h>
#include <conf/firewallconf.h>
#include <conf/rule.h>
#include <driver/drivercommon.h>
#include <log/logentryblockedip.h>
#include <manager/envmanager.h>
#include <util/conf/confappswalker.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confruleswalker.h>
#include <util/fileutil.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>
#include <util/stringutil.h>

#include <mocks/mocksqlitestmt.h>

class ConfUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void ConfUtilTest::SetUp() { }

void ConfUtilTest::TearDown() { }

TEST_F(ConfUtilTest, confWriteRead)
{
    EnvManager envManager;
    FirewallConf conf;

    AddressGroup *inetGroup = conf.inetAddressGroup();

    inetGroup->setIncludeAll(true);
    inetGroup->setExcludeAll(false);

    inetGroup->setIncludeText(QString());
    inetGroup->setExcludeText(NetUtil::localIpNetworksText());

    conf.setAppBlockAll(true);
    conf.setAppAllowAll(false);

    AppGroup *appGroup1 = new AppGroup();
    appGroup1->setName("Base");
    appGroup1->setEnabled(true);
    appGroup1->setPeriodEnabled(true);
    appGroup1->setPeriodFrom("00:00");
    appGroup1->setPeriodTo("12:00");
    appGroup1->setBlockText("System");
    appGroup1->setAllowText("C:\\Program Files\\Skype\\Phone\\Skype.exe\n"
                            "?:\\Utils\\Dev\\Git\\**\n"
                            "D:\\**\\Programs\\**\n");

    AppGroup *appGroup2 = new AppGroup();
    appGroup2->setName("Browser");
    appGroup2->setEnabled(false);
    appGroup2->setAllowText("C:\\Utils\\Firefox\\Bin\\firefox.exe");
    appGroup2->setLimitInEnabled(true);
    appGroup2->setSpeedLimitIn(1024);

    conf.addAppGroup(appGroup1);
    conf.addAppGroup(appGroup2);

    conf.resetEdited(FirewallConf::AllEdited);
    conf.prepareToSave();

    ConfBuffer confBuf;

    if (!confBuf.writeConf(conf, nullptr, envManager)) {
        qCritical() << "Error:" << confBuf.errorMessage();
        Q_UNREACHABLE();
    }

    // Check the buffer
    const char *data = confBuf.data() + DriverCommon::confIoConfOff();

    ASSERT_FALSE(DriverCommon::confIp4InRange(data, 0, true));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetFormatUtil::textToIp4("9.255.255.255")));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetFormatUtil::textToIp4("11.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetFormatUtil::textToIp4("10.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetFormatUtil::textToIp4("169.254.100.100")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetFormatUtil::textToIp4("192.168.255.255")));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetFormatUtil::textToIp4("193.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetFormatUtil::textToIp4("239.255.255.250")));
    ASSERT_TRUE(DriverCommon::confIp6InRange(data, NetFormatUtil::textToIp6("::1")));
    ASSERT_TRUE(DriverCommon::confIp6InRange(data, NetFormatUtil::textToIp6("::2")));
    ASSERT_TRUE(DriverCommon::confIp6InRange(data, NetFormatUtil::textToIp6("::ffff:0:2")));
    ASSERT_FALSE(DriverCommon::confIp6InRange(data, NetFormatUtil::textToIp6("65::")));

    ASSERT_TRUE(DriverCommon::confAppFind(data, "System").found);

    ASSERT_TRUE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Program Files\\Skype\\Phone\\Skype.exe"))
                    .found);
    ASSERT_TRUE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Utils\\Dev\\Git\\git.exe"))
                    .found);
    ASSERT_TRUE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("D:\\Utils\\Dev\\Git\\bin\\git.exe"))
                    .found);
    ASSERT_TRUE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("D:\\My\\Programs\\Test.exe"))
                    .found);

    ASSERT_FALSE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Program Files\\Test.exe"))
                    .found);

    const auto firefoxData = DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Utils\\Firefox\\Bin\\firefox.exe"));
    ASSERT_EQ(int(firefoxData.flags.group_index), 1);
}

TEST_F(ConfUtilTest, checkEnvManager)
{
    EnvManager envManager;

    envManager.setCachedEnvVar("a", "a");
    envManager.setCachedEnvVar("b", "b");
    envManager.setCachedEnvVar("c", "c");

    ASSERT_EQ(envManager.expandString("%%%a%%b%%c%-%c%%b%%a%%%"), "%abc-cba%");

    envManager.setCachedEnvVar("d", "%e%");
    envManager.setCachedEnvVar("e", "%f%");
    envManager.setCachedEnvVar("f", "%d%");

    ASSERT_EQ(envManager.expandString("%d%"), QString());

    envManager.setCachedEnvVar("d", "%e%");
    envManager.setCachedEnvVar("e", "%f%");
    envManager.setCachedEnvVar("f", "%a%");

    ASSERT_EQ(envManager.expandString("%d%"), "a");

    ASSERT_NE(envManager.expandString("%HOME%"), QString());
}

TEST_F(ConfUtilTest, rulesWriteRead)
{
    static Rule g_rules[] = {
        { .ruleId = 1, .ruleText = "1.1.1.1" },
        { .ruleId = 2, .ruleText = "2.2.2.2" },
        { .blocked = true, .ruleId = 3, .ruleText = "3.3.3.3" },
        { .ruleId = 4, .ruleText = "4.4.4.4" },
        { .ruleId = 5, .ruleText = "5.5.5.5" },
        { .ruleType = Rule::PresetRule, .ruleId = 6, .ruleText = "tcp(80)" },
        { .ruleType = Rule::PresetRule, .ruleId = 7, .ruleText = "udp(53)" },
        { .ruleType = Rule::PresetRule, .ruleId = 8, .ruleText = "dir(in)" },
        { .blocked = true, .ruleType = Rule::PresetRule, .ruleId = 9, .ruleText = "area(lan)" },
    };

    struct SubRule
    {
        quint16 ids[2]; // ruleId, subRuleId
    };

    static SubRule g_subRules[] = { // Sub Rules
        // Rule 1
        { 1, 6 }, { 1, 8 }, { 1, 9 },
        // Rule 2
        { 2, 7 },
        // Rule 5
        { 5, 7 }
    };

    class TestRules : public ConfRulesWalker
    {
    public:
        bool walkRules(
                WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const override
        {
            NiceMock<MockSqliteStmt> stmt;
            const int subRulesCount = std::size(g_subRules);
            int subRulesIndex = 0;

            ON_CALL(stmt, step).WillByDefault([&]() -> SqliteStmt::StepResult {
                return (++subRulesIndex < subRulesCount) ? SqliteStmt::StepRow
                                                         : SqliteStmt::StepDone;
            });

            ON_CALL(stmt, columnInt).WillByDefault([&](int column) -> qint32 {
                Q_ASSERT(column >= 0 && column <= 1);
                Q_ASSERT(subRulesIndex > 0 && subRulesIndex <= subRulesCount);

                const SubRule &subRule = g_subRules[subRulesIndex - 1];
                return subRule.ids[column];
            });

            wra.maxRuleId = 9;

            ConfRuleManager::walkRulesMapByStmt(wra, stmt);

            return walkRulesLoop(func);
        }

    private:
        bool walkRulesLoop(const std::function<walkRulesCallback> &func) const
        {
            for (const auto &rule : g_rules) {
                if (!func(rule))
                    return false;
            }

            return true;
        }
    };

    TestRules testRules;

    ConfBuffer confBuf;

    if (!confBuf.writeRules(testRules)) {
        qCritical() << "Error:" << confBuf.errorMessage();
        Q_UNREACHABLE();
    }

    // Check the buffer
    const char *data = confBuf.data();

    // Allowed IP
    {
        const FORT_CONF_META_CONN conn = {
            .inbound = true,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Allowed IP
    {
        const FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("4.4.4.4") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/4));
    }

    // Blocked IP
    {
        const FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("3.3.3.3") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Allowed IP
    {
        const FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_UDP,
            .remote_port = 53,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("2.2.2.2") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/2));
    }

    // Blocked IP
    {
        const FORT_CONF_META_CONN conn = {
            .inbound = false,
            .is_local_net = true,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("3.3.3.3") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/2));
    }
}

TEST_F(ConfUtilTest, rulesOneFilter)
{
    static Rule g_rules[] = {
        { .blocked = true, .ruleId = 1, .ruleText = "1.1.1.1:80" },
    };

    class TestRules : public ConfRulesWalker
    {
    public:
        bool walkRules(
                WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const override
        {
            wra.maxRuleId = 1;

            return walkRulesLoop(func);
        }

    private:
        bool walkRulesLoop(const std::function<walkRulesCallback> &func) const
        {
            for (const auto &rule : g_rules) {
                if (!func(rule))
                    return false;
            }

            return true;
        }
    };

    TestRules testRules;

    ConfBuffer confBuf;

    if (!confBuf.writeRules(testRules)) {
        qCritical() << "Error:" << confBuf.errorMessage();
        Q_UNREACHABLE();
    }

    // Check the buffer
    const char *data = confBuf.data();

    // Blocked IP
    {
        const FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Allowed IP
    {
        const FORT_CONF_META_CONN conn = {
            .inbound = true,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("2.2.2.2") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }
}
