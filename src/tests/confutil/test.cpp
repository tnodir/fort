#include "test.h"

#include "commontest.h"
#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortcommon.h"
#include "util/conf/confutil.h"
#include "util/envmanager.h"
#include "util/fileutil.h"
#include "util/net/netutil.h"

void Test::confWriteRead()
{
    EnvManager envManager;
    FirewallConf conf;

    AddressGroup *inetGroup = conf.inetAddressGroup();

    inetGroup->setIncludeAll(true);
    inetGroup->setExcludeAll(false);

    inetGroup->setIncludeText(QString());
    inetGroup->setExcludeText(
                NetUtil::localIpv4Networks().join('\n')
                );

    conf.setAppBlockAll(true);
    conf.setAppAllowAll(false);

    AppGroup *appGroup1 = new AppGroup();
    appGroup1->setName("Base");
    appGroup1->setEnabled(true);
    appGroup1->setPeriodEnabled(true);
    appGroup1->setPeriodFrom("00:00");
    appGroup1->setPeriodTo("12:00");
    appGroup1->setBlockText(
                "System"
                );
    appGroup1->setAllowText(
                "C:\\Program Files\\Skype\\Phone\\Skype.exe\n"
                "C:\\Utils\\Dev\\Git\\**\n"
                "D:\\**\\Programs\\**\n"
                );

    AppGroup *appGroup2 = new AppGroup();
    appGroup2->setName("Browser");
    appGroup2->setEnabled(false);
    appGroup2->setAllowText(
                "C:\\Utils\\Firefox\\Bin\\firefox.exe"
                );
    appGroup2->setLimitInEnabled(true);
    appGroup2->setSpeedLimitIn(1024);

    conf.addAppGroup(appGroup1);
    conf.addAppGroup(appGroup2);

    ConfUtil confUtil;

    QByteArray buf;
    const int confIoSize = confUtil.write(conf, envManager, buf);
    QVERIFY(confIoSize != 0);

    // Check the buffer
    const char *data = buf.constData() + FortCommon::confIoConfOff();

    QVERIFY(!FortCommon::confIpInRange(data, 0, true));
    QVERIFY(!FortCommon::confIpInRange(data, NetUtil::textToIp4("9.255.255.255")));
    QVERIFY(!FortCommon::confIpInRange(data, NetUtil::textToIp4("11.0.0.0")));
    QVERIFY(FortCommon::confIpInRange(data, NetUtil::textToIp4("10.0.0.0")));
    QVERIFY(FortCommon::confIpInRange(data, NetUtil::textToIp4("169.254.100.100")));
    QVERIFY(FortCommon::confIpInRange(data, NetUtil::textToIp4("192.168.255.255")));
    QVERIFY(!FortCommon::confIpInRange(data, NetUtil::textToIp4("193.0.0.0")));

    QVERIFY(FortCommon::confAppBlocked(
                data, FortCommon::confAppFind(data, "System")));
    QVERIFY(!FortCommon::confAppBlocked(
                data, FortCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("C:\\Program Files\\Skype\\Phone\\Skype.exe"))));
    QVERIFY(!FortCommon::confAppBlocked(
                data, FortCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("C:\\Utils\\Dev\\Git\\**"))));
    QVERIFY(!FortCommon::confAppBlocked(
                data, FortCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("D:\\My\\Programs\\Test.exe"))));
    QVERIFY(FortCommon::confAppBlocked(
                data, FortCommon::confAppFind(
                    data, FileUtil::pathToKernelPath("C:\\Program Files\\Test.exe"))));

    QCOMPARE(FortCommon::confAppPeriodBits(data, 0, 0), 0x01);
    QCOMPARE(FortCommon::confAppPeriodBits(data, 12, 0), 0);

    const quint16 firefoxFlags = FortCommon::confAppFind(
                data, FileUtil::pathToKernelPath("C:\\Utils\\Firefox\\Bin\\firefox.exe"));
    QVERIFY(FortCommon::confAppBlocked(data, firefoxFlags));
    QCOMPARE(1, int(FortCommon::confAppGroupIndex(firefoxFlags)));
}

void Test::checkPeriod()
{
    const quint8 h = 15, m = 35;

    QVERIFY(FortCommon::isTimeInPeriod(h, m, 0,0, 24,0));
    QVERIFY(FortCommon::isTimeInPeriod(h, m, 15,0, 16,0));
    QVERIFY(FortCommon::isTimeInPeriod(h, m, 15,0, 10,0));
    QVERIFY(!FortCommon::isTimeInPeriod(h, m, 15,0, 15,0));
    QVERIFY(!FortCommon::isTimeInPeriod(h, m, 0,0, 15,0));
    QVERIFY(!FortCommon::isTimeInPeriod(h, m, 16,0, 15,0));
    QVERIFY(!FortCommon::isTimeInPeriod(h, m, 24,0, 0,0));
    QVERIFY(!FortCommon::isTimeInPeriod(h, m, 16,0, 14,0));
    QVERIFY(!FortCommon::isTimeInPeriod(h, m, 16,0, 24,0));

    QVERIFY(FortCommon::isTimeInPeriod(h, m, 15,35, 15,37));
    QVERIFY(!FortCommon::isTimeInPeriod(h, m, 15,35, 15,36));
}

void Test::checkEnvManager()
{
    EnvManager envManager;

    envManager.setCachedEnvVar("a", "a");
    envManager.setCachedEnvVar("b", "b");
    envManager.setCachedEnvVar("c", "c");

    QCOMPARE("%abc-cba%", envManager.expandString("%%%a%%b%%c%-%c%%b%%a%%%"));

    envManager.setCachedEnvVar("d", "%e%");
    envManager.setCachedEnvVar("e", "%f%");
    envManager.setCachedEnvVar("f", "%d%");

    QCOMPARE("", envManager.expandString("%d%"));

    envManager.setCachedEnvVar("d", "%e%");
    envManager.setCachedEnvVar("e", "%f%");
    envManager.setCachedEnvVar("f", "%a%");

    QCOMPARE("a", envManager.expandString("%d%"));

    QVERIFY(!envManager.expandString("%HOME%").isEmpty());
}
