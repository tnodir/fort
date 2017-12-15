#include "test.h"

#include <QtTest>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortcommon.h"
#include "util/confutil.h"
#include "util/fileutil.h"
#include "util/net/netutil.h"

void Test::confWriteRead()
{
    FirewallConf conf;

    conf.ipInclude()->setUseAll(true);
    conf.ipExclude()->setUseAll(false);

    conf.setAppBlockAll(true);
    conf.setAppAllowAll(false);

    conf.ipInclude()->setText(QString());
    conf.ipExclude()->setText(
                "10.0.0.0/8\n"
                "127.0.0.0/8\n"
                "169.254.0.0/16\n"
                "172.16.0.0/12\n"
                "192.168.0.0/16\n"
                );

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

    conf.addAppGroup(appGroup1);
    conf.addAppGroup(appGroup2);

    ConfUtil confUtil;

    QByteArray buf;
    const int confSize = confUtil.write(conf, buf);
    QVERIFY(confSize != 0);

    // Check the buffer
    const char *data = buf.constData();

    QVERIFY(!FortCommon::confIpInRange(data, 0, true));
    QVERIFY(!FortCommon::confIpInRange(data, NetUtil::textToIp4("9.255.255.255")));
    QVERIFY(!FortCommon::confIpInRange(data, NetUtil::textToIp4("11.0.0.0")));
    QVERIFY(FortCommon::confIpInRange(data, NetUtil::textToIp4("10.0.0.0")));
    QVERIFY(FortCommon::confIpInRange(data, NetUtil::textToIp4("169.254.100.100")));
    QVERIFY(FortCommon::confIpInRange(data, NetUtil::textToIp4("192.168.255.255")));
    QVERIFY(!FortCommon::confIpInRange(data, NetUtil::textToIp4("193.0.0.0")));

    QVERIFY(FortCommon::confAppBlocked(data, "System"));
    QVERIFY(!FortCommon::confAppBlocked(data, FileUtil::pathToKernelPath("C:\\Program Files\\Skype\\Phone\\Skype.exe").toLower()));
    QVERIFY(!FortCommon::confAppBlocked(data, FileUtil::pathToKernelPath("C:\\Utils\\Dev\\Git\\").toLower()));
    QVERIFY(FortCommon::confAppBlocked(data, FileUtil::pathToKernelPath("C:\\Program Files\\Test.exe").toLower()));
    QVERIFY(FortCommon::confAppBlocked(data, FileUtil::pathToKernelPath("C:\\Utils\\Firefox\\Bin\\firefox.exe").toLower()));
}
