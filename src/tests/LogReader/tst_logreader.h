#pragma once

#include <QDebug>
#include <QFile>
#include <QSignalSpy>

#include <googletest.h>

#include <conf/addressgroup.h>
#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <fortcommon.h>
#include <log/logbuffer.h>
#include <log/logentryblocked.h>
#include <util/conf/confappswalker.h>
#include <util/conf/confutil.h>
#include <util/device.h>
#include <util/envmanager.h>
#include <util/fileutil.h>
#include <util/net/netutil.h>
#include <util/osutil.h>
#include <util/processinfo.h>

class LogBufferTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void LogBufferTest::SetUp() { }

void LogBufferTest::TearDown() { }

namespace {

void validateDriver(Device &device)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int verSize = confUtil.writeVersion(buf);
    ASSERT_NE(verSize, 0);

    ASSERT_TRUE(device.ioctl(FortCommon::ioctlValidate(), buf.data(), verSize));
}

void setConf(Device &device)
{
    EnvManager envManager;
    FirewallConf conf;

    conf.setProvBoot(true);
    conf.setLogBlocked(true);

    // Address Groups
    AddressGroup *inetGroup = conf.inetAddressGroup();

    inetGroup->setIncludeAll(true);
    inetGroup->setExcludeAll(false);

    inetGroup->setExcludeText(NetUtil::localIpv4Networks().join('\n'));

    // Application Groups
    conf.addDefaultAppGroup();

    conf.setAppBlockAll(true);
    conf.setAppAllowAll(false);

    ConfUtil confUtil;

    QByteArray buf;
    const int confIoSize = confUtil.write(conf, nullptr, envManager, buf);
    ASSERT_NE(confIoSize, 0);

    ASSERT_TRUE(device.ioctl(FortCommon::ioctlSetConf(), buf.data(), confIoSize));
}

void printLogs(LogBuffer &buf)
{
    for (;;) {
        const LogEntry::LogType logType = buf.peekEntryType();
        if (logType == LogEntry::TypeNone)
            break;

        ASSERT_EQ(logType, LogEntry::AppBlocked);

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

}

TEST_F(LogBufferTest, LogRead)
{
    Device device;
    ASSERT_TRUE(device.open(FortCommon::deviceName()));

    validateDriver(device);
    setConf(device);

    LogBuffer buf(FortCommon::bufferSize());

    for (;;) {
        int nr;
        QByteArray &array = buf.array();
        ASSERT_TRUE(device.ioctl(
                FortCommon::ioctlGetLog(), nullptr, 0, array.data(), array.size(), &nr));
        buf.reset(nr);
        printLogs(buf);
    }
}
