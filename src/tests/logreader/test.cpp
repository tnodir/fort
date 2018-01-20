#include "test.h"

#include <QtTest>
#include <QDebug>
#include <QFile>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortcommon.h"
#include "log/logbuffer.h"
#include "log/logentryblocked.h"
#include "util/conf/confutil.h"
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

    AddressGroup *inetGroup = conf.inetAddressGroup();

    inetGroup->setIncludeAll(true);
    inetGroup->setExcludeAll(false);

    inetGroup->setExcludeText(
                NetUtil::localIpv4Networks().join('\n')
                );

    conf.setAppBlockAll(true);
    conf.setAppAllowAll(false);

    conf.setLogBlocked(true);

    ConfUtil confUtil;

    QByteArray buf;
    const int confIoSize = confUtil.write(conf, buf);
    QVERIFY(confIoSize != 0);

    QVERIFY(device.ioctl(FortCommon::ioctlSetConf(),
                         buf.data(), confIoSize));
}

void Test::printLogs(LogBuffer &buf)
{
    forever {
        const LogEntry::LogType logType = buf.peekEntryType();
        if (logType == LogEntry::TypeNone)
            break;

        QCOMPARE(logType, LogEntry::AppBlocked);

        LogEntryBlocked entry;

        buf.readEntryBlocked(&entry);

        const quint32 pid = entry.pid();
        QString kernelPath = entry.kernelPath();

        if (kernelPath.isEmpty()) {
            ProcessInfo pi(pid);
            kernelPath = pi.path(true);
        }

        qDebug() << pid << kernelPath << NetUtil::ip4ToText(entry.ip());
    }
}
