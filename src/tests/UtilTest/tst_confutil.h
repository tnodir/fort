#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <conf/addressgroup.h>
#include <conf/appgroup.h>
#include <conf/confrulemanager.h>
#include <conf/firewallconf.h>
#include <conf/rule.h>
#include <driver/drivercommon.h>
#include <log/logentryconn.h>
#include <manager/envmanager.h>
#include <util/conf/confappswalker.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confruleswalker.h>
#include <util/fileutil.h>
#include <util/net/iprange.h>
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

    if (!confBuf.writeConf(conf, nullptr, &envManager)) {
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

    ASSERT_TRUE(DriverCommon::confAppFind(data, "System").flags.found);

    ASSERT_TRUE(DriverCommon::confAppFind(data, "C:\\Program Files\\Skype\\Phone\\Skype.exe")
                    .flags.found);
    ASSERT_TRUE(DriverCommon::confAppFind(data, "C:\\Utils\\Dev\\Git\\git.exe").flags.found);
    ASSERT_TRUE(DriverCommon::confAppFind(data, "D:\\Utils\\Dev\\Git\\bin\\git.exe").flags.found);
    ASSERT_TRUE(DriverCommon::confAppFind(data, "D:\\My\\Programs\\Test.exe").flags.found);

    ASSERT_FALSE(DriverCommon::confAppFind(data, "C:\\Program Files\\Test.exe").flags.found);

    const auto firefoxData =
            DriverCommon::confAppFind(data, "C:\\Utils\\Firefox\\Bin\\firefox.exe");
    ASSERT_EQ(int(firefoxData.group_index), 1);
}

TEST_F(ConfUtilTest, stringCmp)
{
    const auto stringCmp = [](const wchar_t *s1, int n1, const wchar_t *s2, int n2,
                                   UINT16 &common_n) {
        return fort_string_cmp(s1, UINT16(n1), s2, UINT16(n2), &common_n);
    };

    UINT16 common_n;

    // Equal
    ASSERT_EQ(stringCmp(L"abc", 3, L"abc", 3, common_n), 0);
    ASSERT_EQ(common_n, 3);

    // Prefix is less
    ASSERT_LT(stringCmp(L"ab", 2, L"abc", 3, common_n), 0);
    ASSERT_EQ(common_n, 2);

    ASSERT_GT(stringCmp(L"abcd", 4, L"abc", 3, common_n), 0);
    ASSERT_EQ(common_n, 3);

    // Different chars
    ASSERT_LT(stringCmp(L"abx", 3, L"aby", 3, common_n), 0);
    ASSERT_EQ(common_n, 2);

    // UTF-16 code units, not bytes: 'z' (0x007A) < 'я' (0x044F)
    ASSERT_LT(stringCmp(L"z", 1, L"я", 1, common_n), 0);
    ASSERT_EQ(common_n, 0);

    // Only the given length is compared
    ASSERT_EQ(stringCmp(L"abcdef", 3, L"abc", 3, common_n), 0);
    ASSERT_EQ(common_n, 3);

    // Empty
    ASSERT_LT(stringCmp(L"", 0, L"a", 1, common_n), 0);
    ASSERT_EQ(common_n, 0);
}

TEST_F(ConfUtilTest, confAppPrefixFind)
{
    EnvManager envManager;
    FirewallConf conf;

    AppGroup *appGroup = new AppGroup();
    appGroup->setName("Prefixes");
    appGroup->setEnabled(true);
    appGroup->setAllowText("C:\\A\\**\n"
                           "C:\\Я\\**\n");
    appGroup->setBlockText("C:\\A\\B\\**\n"
                           "C:\\A\\C\\**\n"
                           "C:\\A\\D\\**\n"
                           "C:\\A\\Z\\**\n"
                           "C:\\Z\\**\n");
    conf.addAppGroup(appGroup);

    conf.resetEdited(FirewallConf::AllEdited);
    conf.prepareToSave();

    ConfBuffer confBuf;

    if (!confBuf.writeConf(conf, nullptr, &envManager)) {
        qCritical() << "Error:" << confBuf.errorMessage();
        Q_UNREACHABLE();
    }

    const char *data = confBuf.data() + DriverCommon::confIoConfOff();

    const auto appFind = [&](const QString &path) { return DriverCommon::confAppFind(data, path); };

    // Nested prefixes
    ASSERT_TRUE(appFind("C:\\A\\e.exe").flags.found);
    ASSERT_FALSE(appFind("C:\\A\\e.exe").flags.blocked);

    ASSERT_TRUE(appFind("C:\\A\\B\\x.exe").flags.blocked);
    ASSERT_TRUE(appFind("C:\\A\\Z\\y.exe").flags.blocked);

    // Non-Latin prefix is sorted by UTF-16 code units
    ASSERT_TRUE(appFind("C:\\Я\\x.exe").flags.found);
    ASSERT_FALSE(appFind("C:\\Я\\x.exe").flags.blocked);

    ASSERT_TRUE(appFind("C:\\Z\\x.exe").flags.blocked);

    // Path shorter than the prefix
    ASSERT_FALSE(appFind("C:\\A").flags.found);

    ASSERT_FALSE(appFind("C:\\B\\x.exe").flags.found);
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
        { .blocked = true,
                .ruleType = Rule::PresetRule,
                .ruleId = 10,
                .ruleText = "area(localhost)" },
    };

    constexpr int MaxRuleId = 10;

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

            wra.maxRuleId = MaxRuleId;

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
        FORT_CONF_META_CONN conn = {
            .inbound = true,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Allowed IP
    {
        FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("4.4.4.4") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/4));
    }

    // Allowed Port
    {
        FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("3.3.3.3") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
        ASSERT_FALSE(conn.blocked);
    }

    // Allowed IP
    {
        FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_UDP,
            .remote_port = 53,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("2.2.2.2") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/2));
    }

    // Blocked LocalHost
    {
        FORT_CONF_META_CONN conn = {
            .inbound = false,
            .is_loopback = true,
            .ip_proto = IpProto_TCP,
            .remote_port = 3128,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("127.0.0.1") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/10));
        ASSERT_TRUE(conn.blocked);
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
        FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Blocked IP, but Allowed Port
    {
        FORT_CONF_META_CONN conn = {
            .inbound = false,
            .ip_proto = IpProto_TCP,
            .remote_port = 443,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Allowed IP
    {
        FORT_CONF_META_CONN conn = {
            .inbound = true,
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("2.2.2.2") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }
}

TEST_F(ConfUtilTest, rulesTwoFilters)
{
    static Rule g_rules[] = {
        {
                .blocked = true,
                .ruleId = 1,
                .ruleText = "104.21.5.235:443\n"
                            "172.67.154.192\n"
                            "127.0.0.1:=local_ip:dir(IN)\n"
                            "=port(999)\n",
        },
        {
                .blocked = true,
                .ruleId = 2,
                .ruleText = "=ip()",
        },
    };

    class TestRules : public ConfRulesWalker
    {
    public:
        bool walkRules(
                WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const override
        {
            wra.maxRuleId = 2;

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
        FORT_CONF_META_CONN conn = {
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("2.2.2.2") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Blocked IP
    {
        FORT_CONF_META_CONN conn = {
            .ip_proto = IpProto_TCP,
            .remote_port = 443,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("104.21.5.235") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Blocked IP: Equal Local & Remote
    {
        const auto ip4 = NetFormatUtil::textToIp4("127.0.0.1");

        FORT_CONF_META_CONN conn = {
            .inbound = true,
            .ip_proto = IpProto_TCP,
            .local_ip = { .v4 = ip4 },
            .remote_ip = { .v4 = ip4 },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Blocked Port: Equal Local & Remote
    {
        constexpr UINT16 port = 999;

        FORT_CONF_META_CONN conn = {
            .ip_proto = IpProto_UDP,
            .local_port = port,
            .remote_port = port,
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Blocked IP: Any Equal Local & Remote
    {
        const auto ip4 = NetFormatUtil::textToIp4("127.0.0.1");

        FORT_CONF_META_CONN conn = {
            .ip_proto = IpProto_TCP,
            .local_ip = { .v4 = ip4 },
            .remote_ip = { .v4 = ip4 },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/2));
    }
}

TEST_F(ConfUtilTest, ruleExclusiveTerminating)
{
    static Rule g_rules[] = {
        {
                .blocked = false,
                .exclusive = true,
                .terminate = true,
                .terminateBlocked = true,
                .ruleId = 1,
                .ruleText = "dir(OUT):area(LOCALHOST)",
        },
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

    // Allowed IP
    {
        FORT_CONF_META_CONN conn = {
            .is_loopback = true,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("127.0.0.1") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Blocked IP
    {
        FORT_CONF_META_CONN conn = {
            .remote_port = 443,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }
}

TEST_F(ConfUtilTest, ruleFilterActionOption)
{
    static Rule g_rules[] = {
        {
                .blocked = false,
                .ruleId = 1,
                .ruleText = "dir(IN):act(BLOCK)\n"
                            "area(LOCALHOST):act(BLOCK)\n"
                            "{ act(BLOCK) }:opt(LOG, ALERT):port(111)\n",
        },
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

    // Blocked Direction
    {
        FORT_CONF_META_CONN conn = {
            .inbound = true,
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Allowed Direction
    {
        FORT_CONF_META_CONN conn = {
            .inbound = false,
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Blocked IP
    {
        FORT_CONF_META_CONN conn = {
            .is_loopback = true,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("127.0.0.1") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Allowed IP
    {
        FORT_CONF_META_CONN conn = {
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));
    }

    // Action in the block and Options
    {
        FORT_CONF_META_CONN conn = {
            .remote_port = 111,
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(data, &conn, /*ruleId=*/1));

        ASSERT_TRUE(conn.conn_log);
        ASSERT_TRUE(conn.conn_alert);
    }
}

TEST_F(ConfUtilTest, ruleZones)
{
    // Zones: 1 - "10.0.0.0/8", 2 - "10.1.0.0/16"
    constexpr quint32 zone1 = (1 << 0);
    constexpr quint32 zone2 = (1 << 1);

    static Rule g_rules[] = {
        { .blocked = false, .ruleId = 1, .zones = { .accept_mask = zone1 } },
        { .blocked = false, .ruleId = 2, .zones = { .reject_mask = zone2 } },
        { .blocked = false, .ruleId = 3, .zones = { .accept_mask = zone1, .reject_mask = zone2 } },
        {
                .blocked = true,
                .inlineZones = true,
                .ruleId = 4,
                .zones = { .accept_mask = zone1, .reject_mask = zone2 },
                .ruleText = "zones()",
        },
        {
                .blocked = true,
                .inlineZones = true,
                .ruleId = 5,
                .zones = { .accept_mask = zone1, .reject_mask = zone2 },
                .ruleText = "zones(ACCEPTED)",
        },
        {
                .blocked = true,
                .inlineZones = true,
                .ruleId = 6,
                .zones = { .accept_mask = zone1, .reject_mask = zone2 },
                .ruleText = "zones(REJECTED)",
        },
        {
                .blocked = true,
                .inlineZones = true,
                .ruleId = 7,
                .zones = { .reject_mask = zone2 },
                .ruleText = "zones()",
        },
    };

    class TestRules : public ConfRulesWalker
    {
    public:
        bool walkRules(
                WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const override
        {
            wra.maxRuleId = 7;

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

    // Write the zones
    const auto zoneData = [](const QString &text) {
        IpRange ipRange;
        if (!ipRange.fromText(text)) {
            qCritical() << "Error:" << ipRange.errorLineAndMessageDetails();
            Q_UNREACHABLE();
        }

        ConfBuffer zoneBuf;
        zoneBuf.writeZone(ipRange);

        return zoneBuf.buffer();
    };

    const QList<QByteArray> zonesData = { zoneData("10.0.0.0/8"), zoneData("10.1.0.0/16") };
    const quint32 zonesDataSize = zonesData[0].size() + zonesData[1].size();

    ConfBuffer zonesBuf;
    zonesBuf.writeZones(zone1 | zone2, zone1 | zone2, zonesDataSize, zonesData);

    // Check the buffers
    const char *data = confBuf.data();
    const char *zones = zonesBuf.data();

    const auto connFiltered = [&](quint16 ruleId, const char *ip, FORT_CONF_META_CONN &conn) {
        conn = {
            .remote_ip = { .v4 = NetFormatUtil::textToIp4(ip) },
        };

        return DriverCommon::confRulesConnFiltered(data, &conn, ruleId, zones);
    };

    FORT_CONF_META_CONN conn;

    // Accept only: Rule's action
    {
        ASSERT_TRUE(connFiltered(/*ruleId=*/1, "10.2.2.2", conn));
        ASSERT_FALSE(conn.blocked);
        ASSERT_EQ(conn.zone_id, 1);

        ASSERT_FALSE(connFiltered(/*ruleId=*/1, "8.8.8.8", conn));
    }

    // Reject only: Rule's action, if not Rejected
    {
        ASSERT_FALSE(connFiltered(/*ruleId=*/2, "10.1.1.1", conn));

        ASSERT_TRUE(connFiltered(/*ruleId=*/2, "8.8.8.8", conn));
        ASSERT_FALSE(conn.blocked);
        ASSERT_EQ(conn.zone_id, 0);
    }

    // Accept and Reject: Rejected is ignored
    {
        ASSERT_FALSE(connFiltered(/*ruleId=*/3, "10.1.1.1", conn));

        ASSERT_TRUE(connFiltered(/*ruleId=*/3, "10.2.2.2", conn));
        ASSERT_FALSE(conn.blocked);
        ASSERT_EQ(conn.zone_id, 1);

        ASSERT_FALSE(connFiltered(/*ruleId=*/3, "8.8.8.8", conn));
    }

    // Inline Zones: Result
    {
        ASSERT_TRUE(connFiltered(/*ruleId=*/4, "10.2.2.2", conn));
        ASSERT_FALSE(connFiltered(/*ruleId=*/4, "10.1.1.1", conn));
        ASSERT_FALSE(connFiltered(/*ruleId=*/4, "8.8.8.8", conn));
    }

    // Inline Zones: Accepted, even if Rejected
    {
        ASSERT_TRUE(connFiltered(/*ruleId=*/5, "10.2.2.2", conn));
        ASSERT_TRUE(connFiltered(/*ruleId=*/5, "10.1.1.1", conn));
        ASSERT_FALSE(connFiltered(/*ruleId=*/5, "8.8.8.8", conn));
    }

    // Inline Zones: Rejected
    {
        ASSERT_TRUE(connFiltered(/*ruleId=*/6, "10.1.1.1", conn));
        ASSERT_FALSE(connFiltered(/*ruleId=*/6, "10.2.2.2", conn));
        ASSERT_FALSE(connFiltered(/*ruleId=*/6, "8.8.8.8", conn));
    }

    // Inline Zones: Reject only
    {
        ASSERT_FALSE(connFiltered(/*ruleId=*/7, "10.1.1.1", conn));
        ASSERT_TRUE(connFiltered(/*ruleId=*/7, "8.8.8.8", conn));
    }
}

namespace {

class TestRulesWalker : public ConfRulesWalker
{
public:
    explicit TestRulesWalker(const QList<Rule> &rules, quint16 maxRuleId) :
        m_maxRuleId(maxRuleId), m_rules(rules)
    {
    }

    void addRuleSet(quint16 ruleId, const QList<quint16> &subRuleIds)
    {
        const RuleSetInfo ruleSetInfo = {
            .index = quint32(m_ruleSetIds.size()),
            .count = quint8(subRuleIds.size()),
        };

        m_ruleSetMap.insert(ruleId, ruleSetInfo);
        m_ruleSetIds.append(subRuleIds);
    }

    bool walkRules(WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const override
    {
        wra.maxRuleId = m_maxRuleId;
        wra.ruleSetMap = m_ruleSetMap;
        wra.ruleSetIds = m_ruleSetIds;

        for (const auto &rule : m_rules) {
            if (!func(rule))
                return false;
        }

        return true;
    }

private:
    quint16 m_maxRuleId = 0;
    QList<Rule> m_rules;

    ruleset_map_t m_ruleSetMap;
    ruleid_arr_t m_ruleSetIds;
};

QByteArray writeTestRules(const TestRulesWalker &testRules)
{
    ConfBuffer confBuf;

    if (!confBuf.writeRules(testRules)) {
        qCritical() << "Error:" << confBuf.errorMessage();
        Q_UNREACHABLE();
    }

    return confBuf.buffer();
}

QByteArray writeTestZone(const QString &text)
{
    IpRange ipRange;
    if (!ipRange.fromText(text)) {
        qCritical() << "Error:" << ipRange.errorLineAndMessageDetails();
        Q_UNREACHABLE();
    }

    ConfBuffer zoneBuf;
    zoneBuf.writeZone(ipRange);

    return zoneBuf.buffer();
}

QByteArray writeTestZones(quint32 zonesMask, const QList<QByteArray> &zonesData)
{
    quint32 zonesDataSize = 0;
    for (const auto &zoneData : zonesData) {
        zonesDataSize += zoneData.size();
    }

    ConfBuffer zonesBuf;
    zonesBuf.writeZones(zonesMask, zonesMask, zonesDataSize, zonesData);

    return zonesBuf.buffer();
}

}

TEST_F(ConfUtilTest, confValid)
{
    EnvManager envManager;
    FirewallConf conf;

    AddressGroup *inetGroup = conf.inetAddressGroup();
    inetGroup->setIncludeAll(true);
    inetGroup->setExcludeText(NetUtil::localIpNetworksText());

    AppGroup *appGroup = new AppGroup();
    appGroup->setName("Base");
    appGroup->setEnabled(true);
    appGroup->setAllowText("C:\\Program Files\\Skype\\Phone\\Skype.exe\n"
                           "?:\\Utils\\Dev\\Git\\**\n"
                           "D:\\Programs\\**\n");
    conf.addAppGroup(appGroup);

    conf.resetEdited(FirewallConf::AllEdited);
    conf.prepareToSave();

    ConfBuffer confBuf;

    if (!confBuf.writeConf(conf, nullptr, &envManager)) {
        qCritical() << "Error:" << confBuf.errorMessage();
        Q_UNREACHABLE();
    }

    const QByteArray buf = confBuf.buffer();

    const auto confValid = [](const QByteArray &buf, quint32 len) {
        return DriverCommon::confIoValid(buf.data(), len);
    };

    const auto confRef = [](QByteArray &buf) { return &PFORT_CONF_IO(buf.data())->conf; };

    ASSERT_TRUE(confValid(buf, buf.size()));

    // Truncated
    ASSERT_FALSE(confValid(buf, buf.size() - 8));
    ASSERT_FALSE(confValid(buf, DriverCommon::confIoConfOff()));

    // Unordered offsets
    {
        QByteArray badBuf = buf;
        PFORT_CONF conf = confRef(badBuf);
        conf->wild_apps_off = conf->exe_apps_off + 4;

        ASSERT_FALSE(confValid(badBuf, badBuf.size()));
    }

    // Too many apps
    {
        QByteArray badBuf = buf;
        PFORT_CONF conf = confRef(badBuf);
        ASSERT_NE(conf->exe_apps_n, 0);
        ++conf->exe_apps_n;

        ASSERT_FALSE(confValid(badBuf, badBuf.size()));
    }

    // Invalid app's path length
    {
        QByteArray badBuf = buf;
        PFORT_CONF conf = confRef(badBuf);
        PFORT_APP_ENTRY app_entry = PFORT_APP_ENTRY(conf->data + conf->exe_apps_off);
        app_entry->path_len = 0xFFFE;

        ASSERT_FALSE(confValid(badBuf, badBuf.size()));
    }

    // Invalid app's group index
    {
        QByteArray badBuf = buf;
        PFORT_CONF conf = confRef(badBuf);
        PFORT_APP_ENTRY app_entry = PFORT_APP_ENTRY(conf->data + conf->exe_apps_off);
        app_entry->app_data.group_index = FORT_CONF_GROUP_MAX;

        ASSERT_FALSE(confValid(badBuf, badBuf.size()));
    }

    // Invalid address group's offset
    {
        QByteArray badBuf = buf;
        PFORT_CONF conf = confRef(badBuf);
        quint32 *addr_group_offsets = (quint32 *) (conf->data + conf->addr_groups_off);
        addr_group_offsets[1] = conf->wild_apps_off;

        ASSERT_FALSE(confValid(badBuf, badBuf.size()));
    }
}

TEST_F(ConfUtilTest, zonesValid)
{
    constexpr quint32 zone1 = (1 << 0);
    constexpr quint32 zone2 = (1 << 1);

    const QByteArray zone1Data = writeTestZone("10.0.0.0/8\n::1");
    const QByteArray zone2Data = writeTestZone("10.1.0.0/16");

    const QByteArray buf = writeTestZones(zone1 | zone2, { zone1Data, zone2Data });

    const auto zonesValid = [](const QByteArray &buf, quint32 len) {
        return DriverCommon::confZonesValid(buf.data(), len);
    };

    ASSERT_TRUE(zonesValid(buf, buf.size()));

    // Truncated
    ASSERT_FALSE(zonesValid(buf, buf.size() - 4));

    // Invalid zone's offset
    {
        QByteArray badBuf = buf;
        PFORT_CONF_ZONES zones = PFORT_CONF_ZONES(badBuf.data());
        zones->addr_off[1] = badBuf.size();

        ASSERT_FALSE(zonesValid(badBuf, badBuf.size()));
    }

    // Too many addresses
    {
        QByteArray badBuf = buf;
        PFORT_CONF_ZONES zones = PFORT_CONF_ZONES(badBuf.data());
        PFORT_CONF_ADDR_LIST addr_list = PFORT_CONF_ADDR_LIST(zones->data + zones->addr_off[0]);
        addr_list->ip_n = FORT_CONF_IP_MAX + 1;

        ASSERT_FALSE(zonesValid(badBuf, badBuf.size()));
    }

    // Migrated zone's data without IPv6 addresses
    {
        PCFORT_CONF_ADDR_LIST addr_list = PCFORT_CONF_ADDR_LIST(zone2Data.data());
        const QByteArray oldZone2Data =
                zone2Data.left(FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n));

        const QByteArray migratedBuf = writeTestZones(zone1 | zone2, { zone1Data, oldZone2Data });

        ASSERT_TRUE(zonesValid(migratedBuf, migratedBuf.size()));
    }
}

TEST_F(ConfUtilTest, rulesValid)
{
    // Nested lists of the max depth
    QString nestedText = "1.1.1.1:port(80)\n2.2.2.2";
    for (int i = 0; i < FORT_CONF_RULE_FILTER_DEPTH_MAX; ++i) {
        nestedText = QString("{ %1 }:port(80)\n3.3.3.%2").arg(nestedText).arg(i + 1);
    }

    const QList<Rule> rules = {
        { .ruleId = 1, .ruleText = "1.1.1.1:80\ntcp(80-90)\nudp(53)\ndir(in):area(lan)" },
        { .blocked = true, .ruleId = 3, .ruleText = nestedText },
        { .ruleId = 4, .zones = { .accept_mask = 1 } },
    };

    const QByteArray buf = writeTestRules(TestRulesWalker(rules, /*maxRuleId=*/4));

    const auto rulesValid = [](const QByteArray &buf, quint32 len) {
        return DriverCommon::confRulesValid(buf.data(), len);
    };

    const auto ruleRef = [](QByteArray &buf, quint16 ruleId) {
        PFORT_CONF_RULES rules = PFORT_CONF_RULES(buf.data());
        const quint32 *rule_offsets = (const quint32 *) rules->data - 1;

        return PFORT_CONF_RULE(rules->data + rule_offsets[ruleId]);
    };

    const auto ruleFilterRef = [&](QByteArray &buf, quint16 ruleId) {
        PFORT_CONF_RULE rule = ruleRef(buf, ruleId);

        return PFORT_CONF_RULE_FILTER((char *) rule + FORT_CONF_RULE_SIZE(rule));
    };

    ASSERT_TRUE(rulesValid(buf, buf.size()));

    // Absent rule
    {
        FORT_CONF_META_CONN conn = {
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_FALSE(DriverCommon::confRulesConnFiltered(buf.data(), &conn, /*ruleId=*/2));
    }

    // Nested lists
    {
        FORT_CONF_META_CONN conn = {
            .ip_proto = IpProto_TCP,
            .remote_port = 80,
            .remote_ip = { .v4 = NetFormatUtil::textToIp4("1.1.1.1") },
        };

        ASSERT_TRUE(DriverCommon::confRulesConnBlocked(buf.data(), &conn, /*ruleId=*/3));
    }

    // Truncated
    ASSERT_FALSE(rulesValid(buf, buf.size() - 4));

    // Too many rules
    {
        QByteArray badBuf = buf;
        PFORT_CONF_RULES rules = PFORT_CONF_RULES(badBuf.data());
        rules->max_rule_id = FORT_CONF_RULE_MAX + 1;

        ASSERT_FALSE(rulesValid(badBuf, badBuf.size()));
    }

    // Overlapped rules
    {
        QByteArray badBuf = buf;
        PFORT_CONF_RULES rules = PFORT_CONF_RULES(badBuf.data());
        quint32 *rule_offsets = (quint32 *) rules->data - 1;
        rule_offsets[3] = rule_offsets[1];

        ASSERT_FALSE(rulesValid(badBuf, badBuf.size()));
    }

    // Rule's offset in the rules' offsets
    {
        QByteArray badBuf = buf;
        PFORT_CONF_RULES rules = PFORT_CONF_RULES(badBuf.data());
        quint32 *rule_offsets = (quint32 *) rules->data - 1;
        rule_offsets[1] = 1;

        ASSERT_FALSE(rulesValid(badBuf, badBuf.size()));
    }

    // Zero filter's size
    {
        QByteArray badBuf = buf;
        ruleFilterRef(badBuf, 1)->size = 0;

        ASSERT_FALSE(rulesValid(badBuf, badBuf.size()));
    }

    // Too big filter's size
    {
        QByteArray badBuf = buf;
        ruleFilterRef(badBuf, 3)->size += 4;

        ASSERT_FALSE(rulesValid(badBuf, badBuf.size()));
    }

    // Invalid list's filter
    {
        QByteArray badBuf = buf;
        PFORT_CONF_RULE_FILTER rule_filter = ruleFilterRef(badBuf, 1);
        ASSERT_EQ(rule_filter->type, FORT_RULE_FILTER_TYPE_LIST_OR);

        PFORT_CONF_RULE_FILTER sub_filter = rule_filter + 1;
        sub_filter->size = rule_filter->size;

        ASSERT_FALSE(rulesValid(badBuf, badBuf.size()));
    }
}

TEST_F(ConfUtilTest, ruleSetsLoop)
{
    const QList<Rule> rules = {
        { .ruleId = 1 },
        { .ruleId = 2 },
    };

    TestRulesWalker testRules(rules, /*maxRuleId=*/2);
    testRules.addRuleSet(1, { 2 });
    testRules.addRuleSet(2, { 1 });

    const QByteArray buf = writeTestRules(testRules);

    ASSERT_TRUE(DriverCommon::confRulesValid(buf.data(), buf.size()));

    FORT_CONF_META_CONN conn = {};

    ASSERT_FALSE(DriverCommon::confRulesConnFiltered(buf.data(), &conn, /*ruleId=*/1));
}
