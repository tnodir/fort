#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <driver/drivercommon.h>
#include <log/logbuffer.h>
#include <log/logentryapp.h>
#include <log/logentryconn.h>
#include <log/logentrytime.h>
#include <util/dateutil.h>

namespace {

ip_addr_t makeIp6(int v)
{
    ip_addr_t ip;
    ip.v6.addr32[0] = v;
    ip.v6.addr32[1] = v + 1;
    ip.v6.addr32[2] = v + 2;
    ip.v6.addr32[3] = v + 3;
    return ip;
}

int compareIp6(ip_addr_t lhs, ip_addr_t rhs)
{
    return memcmp(lhs.v6.data, rhs.v6.data, sizeof(ip6_addr_t));
}

}

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

    const int entrySize = DriverCommon::logAppHeaderSize() + pathSize * sizeof(wchar_t);

    const int testCount = 3;

    LogBuffer buf(entrySize * testCount);

    const quint32 pid = 1;
    LogEntryApp entry(pid, path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        buf.writeEntryApp(&entry);
    }

    // Read
    int readCount = 0;
    while (buf.peekEntryType() == FORT_LOG_TYPE_APP) {
        buf.readEntryApp(&entry);

        ASSERT_EQ(entry.type(), FORT_LOG_TYPE_APP);
        ASSERT_EQ(entry.pid(), pid);
        ASSERT_EQ(entry.kernelPath(), path);

        ++readCount;
    }
    ASSERT_EQ(readCount, testCount);
}

TEST_F(LogBufferTest, blockedIp4WriteRead)
{
    const QString path("C:\\test\\");

    const int pathSize = path.size();
    ASSERT_EQ(pathSize, 8);

    const int entrySize = DriverCommon::logConnHeaderSize() + pathSize * sizeof(wchar_t);

    const int testCount = 3;

    LogBuffer buf(entrySize * testCount);

    LogEntryConn entry;
    entry.setKernelPath(path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        int v = i;
        entry.setInbound((v & 1) != 0);
        entry.setReason(++v);
        entry.setIpProto(++v);
        entry.setLocalPort(++v);
        entry.setRemotePort(++v);
        entry.setLocalIp4(++v);
        entry.setRemoteIp4(++v);
        entry.setPid(++v);

        buf.writeEntryConn(&entry);
    }

    // Read
    int index = 0;
    while (buf.peekEntryType() == FORT_LOG_TYPE_CONN) {
        buf.readEntryConn(&entry);

        int v = index++;
        ASSERT_EQ(entry.type(), FORT_LOG_TYPE_CONN);
        ASSERT_FALSE(entry.isIPv6());
        ASSERT_EQ(entry.inbound(), (v & 1) != 0);
        ASSERT_EQ(entry.reason(), ++v);
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

TEST_F(LogBufferTest, blockedIp6WriteRead)
{
    const QString path("C:\\test6\\");

    const int pathSize = path.size();
    ASSERT_EQ(pathSize, 9);

    constexpr bool isIPv6 = true;
    const int entrySize = DriverCommon::logConnHeaderSize(isIPv6) + pathSize * sizeof(wchar_t);

    const int testCount = 3;

    LogBuffer buf(entrySize * testCount);

    LogEntryConn entry;
    entry.setIsIPv6(isIPv6);
    entry.setKernelPath(path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        int v = i;
        entry.setInbound((v & 1) != 0);
        entry.setReason(++v);
        entry.setIpProto(++v);
        entry.setLocalPort(++v);
        entry.setRemotePort(++v);
        entry.setLocalIp(makeIp6(++v));
        entry.setRemoteIp(makeIp6(++v));
        entry.setPid(++v);

        buf.writeEntryConn(&entry);
    }

    // Read
    int index = 0;
    while (buf.peekEntryType() == FORT_LOG_TYPE_CONN) {
        buf.readEntryConn(&entry);

        int v = index++;
        ASSERT_EQ(entry.type(), FORT_LOG_TYPE_CONN);
        ASSERT_TRUE(entry.isIPv6());
        ASSERT_EQ(entry.inbound(), (v & 1) != 0);
        ASSERT_EQ(entry.reason(), ++v);
        ASSERT_EQ(entry.ipProto(), ++v);
        ASSERT_EQ(entry.localPort(), ++v);
        ASSERT_EQ(entry.remotePort(), ++v);
        ASSERT_EQ(compareIp6(entry.localIp(), makeIp6(++v)), 0);
        ASSERT_EQ(compareIp6(entry.remoteIp(), makeIp6(++v)), 0);
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
