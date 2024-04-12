#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <conf/addressgroup.h>
#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
#include <log/logentryblockedip.h>
#include <manager/envmanager.h>
#include <util/conf/confappswalker.h>
#include <util/conf/confutil.h>
#include <util/fileutil.h>
#include <util/net/netutil.h>

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

    conf.resetEdited(true);
    conf.prepareToSave();

    ConfUtil confUtil;

    const int confIoSize = confUtil.write(conf, nullptr, envManager);
    if (confIoSize == 0) {
        qCritical() << "Error:" << confUtil.errorMessage();
        ASSERT_NE(confIoSize, 0);
    }

    // Check the buffer
    const char *data = confUtil.data() + DriverCommon::confIoConfOff();

    ASSERT_FALSE(DriverCommon::confIp4InRange(data, 0, true));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("9.255.255.255")));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("11.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("10.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("169.254.100.100")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("192.168.255.255")));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("193.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("239.255.255.250")));
    ASSERT_TRUE(DriverCommon::confIp6InRange(data, NetUtil::textToIp6("::2")));
    ASSERT_TRUE(DriverCommon::confIp6InRange(data, NetUtil::textToIp6("::ffff:0:2")));
    ASSERT_FALSE(DriverCommon::confIp6InRange(data, NetUtil::textToIp6("65::")));

    qint8 blockReason = FORT_BLOCK_REASON_UNKNOWN;

    ASSERT_TRUE(DriverCommon::confAppBlocked(
            data, DriverCommon::confAppFind(data, "System"), &blockReason));
    ASSERT_EQ(blockReason, FORT_BLOCK_REASON_APP_GROUP_FOUND);

    ASSERT_FALSE(DriverCommon::confAppBlocked(data,
            DriverCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("C:\\Program Files\\Skype\\Phone\\Skype.exe")),
            &blockReason));
    ASSERT_FALSE(DriverCommon::confAppBlocked(data,
            DriverCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("C:\\Utils\\Dev\\Git\\git.exe")),
            &blockReason));
    ASSERT_FALSE(DriverCommon::confAppBlocked(data,
            DriverCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("D:\\Utils\\Dev\\Git\\bin\\git.exe")),
            &blockReason));
    ASSERT_FALSE(DriverCommon::confAppBlocked(data,
            DriverCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("D:\\My\\Programs\\Test.exe")),
            &blockReason));

    ASSERT_TRUE(DriverCommon::confAppBlocked(data,
            DriverCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("C:\\Program Files\\Test.exe")),
            &blockReason));
    ASSERT_EQ(blockReason, FORT_BLOCK_REASON_FILTER_MODE);

    ASSERT_EQ(DriverCommon::confAppPeriodBits(data, 0, 0), 0x01);
    ASSERT_EQ(DriverCommon::confAppPeriodBits(data, 12, 0), 0);

    const quint16 firefoxFlags = DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Utils\\Firefox\\Bin\\firefox.exe"));
    ASSERT_TRUE(DriverCommon::confAppBlocked(data, firefoxFlags, &blockReason));
    ASSERT_EQ(blockReason, FORT_BLOCK_REASON_APP_GROUP_FOUND);
    ASSERT_EQ(int(DriverCommon::confAppGroupIndex(firefoxFlags)), 1);
}

TEST_F(ConfUtilTest, checkPeriod)
{
    const quint8 h = 15, m = 35;

    ASSERT_TRUE(DriverCommon::isTimeInPeriod(h, m, 0, 0, 24, 0));
    ASSERT_TRUE(DriverCommon::isTimeInPeriod(h, m, 15, 0, 16, 0));
    ASSERT_TRUE(DriverCommon::isTimeInPeriod(h, m, 15, 0, 10, 0));
    ASSERT_FALSE(DriverCommon::isTimeInPeriod(h, m, 15, 0, 15, 0));
    ASSERT_FALSE(DriverCommon::isTimeInPeriod(h, m, 0, 0, 15, 0));
    ASSERT_FALSE(DriverCommon::isTimeInPeriod(h, m, 16, 0, 15, 0));
    ASSERT_FALSE(DriverCommon::isTimeInPeriod(h, m, 24, 0, 0, 0));
    ASSERT_FALSE(DriverCommon::isTimeInPeriod(h, m, 16, 0, 14, 0));
    ASSERT_FALSE(DriverCommon::isTimeInPeriod(h, m, 16, 0, 24, 0));

    ASSERT_TRUE(DriverCommon::isTimeInPeriod(h, m, 15, 35, 15, 37));
    ASSERT_TRUE(!DriverCommon::isTimeInPeriod(h, m, 15, 35, 15, 36));
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
