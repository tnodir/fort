#pragma once

#include <QDebug>
#include <QFile>
#include <QSignalSpy>

#include <googletest.h>

#include <conf/addressgroup.h>
#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
#include <log/logbuffer.h>
#include <log/logentryconn.h>
#include <log/logentrytime.h>
#include <manager/envmanager.h>
#include <util/conf/confappswalker.h>
#include <util/conf/confbuffer.h>
#include <util/device.h>
#include <util/fileutil.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>
#include <util/osutil.h>
#include <util/processinfo.h>

class LogReaderTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void LogReaderTest::SetUp() { }

void LogReaderTest::TearDown() { }

namespace {

void validateDriver(Device &device)
{
    ConfBuffer confBuf;

    confBuf.writeVersion();
    ASSERT_FALSE(confBuf.buffer().isEmpty());

    ASSERT_TRUE(device.ioctl(DriverCommon::ioctlValidate(), confBuf.data()));
}

void setConf(Device &device)
{
    EnvManager envManager;
    FirewallConf conf;

    conf.setBootFilter(true);
    conf.setLogBlockedConn(true);

    // Address Groups
    AddressGroup *inetGroup = conf.inetAddressGroup();

    inetGroup->setIncludeAll(true);
    inetGroup->setExcludeAll(false);

    inetGroup->setExcludeText(NetUtil::localIpNetworksText());

    // Application Groups
    conf.addDefaultAppGroup();

    conf.setAppBlockAll(true);
    conf.setAppAllowAll(false);

    ConfBuffer confBuf;

    ASSERT_TRUE(confBuf.writeConf(conf, nullptr, &envManager));

    auto confData = confBuf.buffer();
    ASSERT_TRUE(device.ioctl(DriverCommon::ioctlSetConf(), confData.data(), confData.size()));
}

void printLogs(LogBuffer &buf)
{
    for (;;) {
        const FortLogType logType = buf.peekEntryType();
        if (logType == FORT_LOG_TYPE_NONE)
            break;

        if (logType == FORT_LOG_TYPE_TIME) {
            LogEntryTime entry;
            buf.readEntryTime(&entry);
            continue;
        }

        ASSERT_EQ(logType, FORT_LOG_TYPE_CONN);

        LogEntryConn entry;
        buf.readEntryConn(&entry);

        qDebug() << entry.pid() << entry.kernelPath() << entry.path()
                 << NetFormatUtil::ipToText(entry.remoteIp()) << entry.remotePort();
    }
}

}

TEST_F(LogReaderTest, logRead)
{
    Device device;
    ASSERT_TRUE(device.open(DriverCommon::deviceName()));

    validateDriver(device);
    setConf(device);

    LogBuffer buf(DriverCommon::bufferSize());

    for (;;) {
        qsizetype nr = 0;
        QByteArray &array = buf.array();
        ASSERT_TRUE(device.ioctl(
                DriverCommon::ioctlGetLog(), nullptr, 0, array.data(), array.size(), &nr));
        buf.reset(nr);
        printLogs(buf);
    }
}
