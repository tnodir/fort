#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <driver/drivercommon.h>
#include <log/logbuffer.h>
#include <log/logentryblocked.h>
#include <log/logentryblockedip.h>
#include <log/logentrytime.h>
#include <util/dateutil.h>

class LogBufferTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void LogBufferTest::SetUp() { }

void LogBufferTest::TearDown() { }

TEST_F(LogBufferTest, blockedWriteRead)
{
    const QString path("C:\\test\\");

    const int pathSize = path.size();
    ASSERT_EQ(pathSize, 8);

    const int entrySize = DriverCommon::logBlockedHeaderSize() + pathSize * sizeof(wchar_t);

    const int testCount = 3;

    LogBuffer buf(entrySize * testCount);

    const quint32 pid = 1;
    LogEntryBlocked entry(pid, path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        buf.writeEntryBlocked(&entry);
    }

    // Read
    int readCount = 0;
    while (buf.peekEntryType() == FORT_LOG_TYPE_BLOCKED) {
        buf.readEntryBlocked(&entry);

        ASSERT_EQ(entry.type(), FORT_LOG_TYPE_BLOCKED);
        ASSERT_EQ(entry.pid(), pid);
        ASSERT_EQ(entry.kernelPath(), path);

        ++readCount;
    }
    ASSERT_EQ(readCount, testCount);
}

TEST_F(LogBufferTest, blockedIpWriteRead)
{
    const QString path("C:\\test\\");

    const int pathSize = path.size();
    ASSERT_EQ(pathSize, 8);

    const int entrySize = DriverCommon::logBlockedIpHeaderSize() + pathSize * sizeof(wchar_t);

    const int testCount = 3;

    LogBuffer buf(entrySize * testCount);

    LogEntryBlockedIp entry;
    entry.setKernelPath(path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        int v = i;
        entry.setInbound((v & 1) != 0);
        entry.setBlockReason(++v);
        entry.setIpProto(++v);
        entry.setLocalPort(++v);
        entry.setRemotePort(++v);
        entry.setLocalIp4(++v);
        entry.setRemoteIp4(++v);
        entry.setPid(++v);

        buf.writeEntryBlockedIp(&entry);
    }

    // Read
    int index = 0;
    while (buf.peekEntryType() == FORT_LOG_TYPE_BLOCKED_IP) {
        buf.readEntryBlockedIp(&entry);

        int v = index++;
        ASSERT_EQ(entry.type(), FORT_LOG_TYPE_BLOCKED_IP);
        ASSERT_FALSE(entry.isIPv6());
        ASSERT_EQ(entry.inbound(), (v & 1) != 0);
        ASSERT_EQ(entry.blockReason(), ++v);
        ASSERT_EQ(entry.ipProto(), ++v);
        ASSERT_EQ(entry.localPort(), ++v);
        ASSERT_EQ(entry.remotePort(), ++v);
        ASSERT_EQ(entry.localIp4(), ++v);
        ASSERT_EQ(entry.remoteIp4(), ++v);
        ASSERT_EQ(entry.pid(), ++v);
        ASSERT_EQ(entry.kernelPath(), path);
    }
    ASSERT_EQ(index, testCount);
}

TEST_F(LogBufferTest, timeWriteRead)
{
    const int entrySize = DriverCommon::logTimeSize();

    LogBuffer buf(entrySize);

    const qint64 unixTime = DateUtil::getUnixTime();
    LogEntryTime entry(unixTime);

    // Write
    buf.writeEntryTime(&entry);

    // Read
    ASSERT_EQ(buf.peekEntryType(), FORT_LOG_TYPE_TIME);
    buf.readEntryTime(&entry);
    ASSERT_EQ(entry.unixTime(), unixTime);
}
