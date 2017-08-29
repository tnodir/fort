#include "test.h"

#include <QtTest>

#include "conf/appgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortcommon.h"
#include "util/confutil.h"
#include "util/fileutil.h"
#include "util/netutil.h"

void Test::confWriteRead()
{
    FirewallConf conf;

    conf.setIpIncludeAll(true);
    conf.setIpExcludeAll(false);

    conf.setAppLogBlocked(true);
    conf.setAppBlockAll(false);
    conf.setAppAllowAll(false);

    conf.setIpIncludeText(QString());
    conf.setIpExcludeText(
                "10.0.0.0/24\n"
                "127.0.0.0/24\n"
                "169.254.0.0/16\n"
                "172.16.0.0/20\n"
                "192.168.0.0/16\n"
                );

    AppGroup *appGroup1 = new AppGroup();
    appGroup1->setName("Base");
    appGroup1->setEnabled(true);
    appGroup1->setBlockText(
                "System"
                );
    appGroup1->setAllowText(
                "C:\\Programs\\Skype\\Phone\\Skype.exe\n"
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
    QVERIFY(confUtil.write(conf, buf));

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
    QVERIFY(!FortCommon::confAppBlocked(data, FileUtil::pathToDosPath("C:\\programs\\skype\\phone\\skype.exe")));
    QVERIFY(!FortCommon::confAppBlocked(data, FileUtil::pathToDosPath("C:\\utils\\dev\\git\\bin\\git.exe")));
}
