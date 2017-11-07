#include "test.h"

#include <QtTest>
#include <QDebug>
#include <QFile>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortcommon.h"
#include "log/logbuffer.h"
#include "log/logentry.h"
#include "util/confutil.h"
#include "util/device.h"
#include "util/fileutil.h"
#include "util/net/netutil.h"
#include "util/osutil.h"
#include "util/processinfo.h"

void Test::logRead()
{
    Device device;
    QVERIFY(device.open(FortCommon::deviceName()));

    setConf(device);

    LogBuffer buf(FortCommon::bufferSize());

    forever {
        int nr;
        QByteArray &array = buf.array();
        QVERIFY(device.ioctl(FortCommon::ioctlGetLog(), nullptr, 0,
                             array.data(), array.size(), &nr));
        buf.reset(nr);
        printLogs(buf);
    }
}

void Test::setConf(Device &device)
{
    FirewallConf conf;

    conf.setProvBoot(true);

    conf.ipInclude()->setUseAll(true);
    conf.ipExclude()->setUseAll(false);

    conf.setAppBlockAll(true);
    conf.setAppAllowAll(false);

    conf.setLogBlocked(true);

    conf.ipExclude()->setText(
                "10.0.0.0/8\n"
                "127.0.0.0/8\n"
                "169.254.0.0/16\n"
                "172.16.0.0/12\n"
                "192.168.0.0/16\n"
                );

    ConfUtil confUtil;

    QByteArray buf;
    const int confSize = confUtil.write(conf, buf);
    QVERIFY(confSize != 0);

    QVERIFY(device.ioctl(FortCommon::ioctlSetConf(),
                         buf.data(), confSize));
}

void Test::printLogs(LogBuffer &buf)
{
    LogEntry entry;

    while (buf.read(&entry)) {
        const quint32 pid = entry.pid();
        QString kernelPath = entry.kernelPath();

        if (kernelPath.isEmpty()) {
            ProcessInfo pi(pid);
            kernelPath = pi.path(true);
        }

        qDebug() << pid << kernelPath << NetUtil::ip4ToText(entry.ip());
    }
}
