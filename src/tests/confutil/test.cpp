#include "test.h"

#include <QtTest>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortcommon.h"
#include "util/conf/confutil.h"
#include "util/fileutil.h"
#include "util/net/netutil.h"

void Test::confWriteRead()
{
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
    appGroup1->setBlockText(
                "System"
                );
    appGroup1->setAllowText(
                "C:\\Program Files\\Skype\\Phone\\Skype.exe\n"
                "C:\\Utils\\Dev\\Git\\\n"
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
    const int confIoSize = confUtil.write(conf, buf);
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
                data, FortCommon::confAppIndex(data, "System")));
    QVERIFY(!FortCommon::confAppBlocked(
                data, FortCommon::confAppIndex(
                    data, FileUtil::pathToKernelPath("C:\\Program Files\\Skype\\Phone\\Skype.exe").toLower())));
    QVERIFY(!FortCommon::confAppBlocked(
                data, FortCommon::confAppIndex(
                    data, FileUtil::pathToKernelPath("C:\\Utils\\Dev\\Git\\").toLower())));
    QVERIFY(FortCommon::confAppBlocked(
                data, FortCommon::confAppIndex(
                    data, FileUtil::pathToKernelPath("C:\\Program Files\\Test.exe").toLower())));

    const int firefoxIndex = FortCommon::confAppIndex(
                data, FileUtil::pathToKernelPath("C:\\Utils\\Firefox\\Bin\\firefox.exe").toLower());
    QVERIFY(FortCommon::confAppBlocked(data, firefoxIndex));
    QCOMPARE(1, int(FortCommon::confAppGroupIndex(data, firefoxIndex)));
}
