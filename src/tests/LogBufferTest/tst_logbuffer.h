#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <fortcommon.h>
#include <log/logbuffer.h>
#include <log/logentryblocked.h>
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

    const quint8 proto = 6;
    const quint16 port = 3;
    const quint32 ip = 1, pid = 2;
    LogEntryBlocked entry(ip, port, proto, pid, path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        buf.writeEntryBlocked(&entry);
    }

    // Read
    int readCount = 0;
    while (buf.peekEntryType() == LogEntry::AppBlocked) {
        buf.readEntryBlocked(&entry);

        ASSERT_EQ(entry.type(), LogEntry::AppBlocked);
        ASSERT_EQ(entry.ip(), ip);
        ASSERT_EQ(entry.port(), port);
        ASSERT_EQ(entry.proto(), proto);
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
