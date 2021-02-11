#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <fortcommon.h>
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

TEST_F(LogBufferTest, BlockedWriteRead)
{
    const QString path("C:\\test\\");

    const int pathSize = path.size();
    ASSERT_EQ(pathSize, 8);

    const int entrySize = FortCommon::logBlockedHeaderSize() + pathSize * sizeof(wchar_t);

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
    while (buf.peekEntryType() == LogEntry::AppBlocked) {
        buf.readEntryBlocked(&entry);

        ASSERT_EQ(entry.type(), LogEntry::AppBlocked);
        ASSERT_EQ(entry.pid(), pid);
        ASSERT_EQ(entry.kernelPath(), path);

        ++readCount;
    }
    ASSERT_EQ(readCount, testCount);
}

TEST_F(LogBufferTest, BlockedIpWriteRead)
{
    const QString path("C:\\test\\");

    const int pathSize = path.size();
    ASSERT_EQ(pathSize, 8);

    const int entrySize = FortCommon::logBlockedIpHeaderSize() + pathSize * sizeof(wchar_t);

    const int testCount = 3;

    LogBuffer buf(entrySize * testCount);

    const quint8 blockReason = 7;
    const quint8 proto = 6;
    const quint16 localPort = 5;
    const quint16 remotePort = 4;
    const quint32 localIp = 3;
    const quint32 remoteIp = 2;
    const quint32 pid = 1;
    LogEntryBlockedIp entry(
            blockReason, proto, localPort, remotePort, localIp, remoteIp, pid, path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        buf.writeEntryBlockedIp(&entry);
    }

    // Read
    int readCount = 0;
    while (buf.peekEntryType() == LogEntry::AppBlockedIp) {
        buf.readEntryBlockedIp(&entry);

        ASSERT_EQ(entry.type(), LogEntry::AppBlockedIp);
        ASSERT_EQ(entry.proto(), proto);
        ASSERT_EQ(entry.localPort(), localPort);
        ASSERT_EQ(entry.remotePort(), remotePort);
        ASSERT_EQ(entry.localIp(), localIp);
        ASSERT_EQ(entry.remoteIp(), remoteIp);
        ASSERT_EQ(entry.pid(), pid);
        ASSERT_EQ(entry.kernelPath(), path);

        ++readCount;
    }
    ASSERT_EQ(readCount, testCount);
}

TEST_F(LogBufferTest, TimeWriteRead)
{
    const int entrySize = FortCommon::logTimeSize();

    LogBuffer buf(entrySize);

    const qint64 unixTime = DateUtil::getUnixTime();
    LogEntryTime entry(unixTime);

    // Write
    buf.writeEntryTime(&entry);

    // Read
    ASSERT_EQ(buf.peekEntryType(), LogEntry::Time);
    buf.readEntryTime(&entry);
    ASSERT_EQ(entry.unixTime(), unixTime);
}
