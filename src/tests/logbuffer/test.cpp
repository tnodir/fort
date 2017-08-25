#include "test.h"

#include <QtTest>

#include "firewallLog/logbuffer.h"
#include "firewallLog/logentry.h"
#include "fortcommon.h"

void Test::logWriteRead()
{
    const QString path("C:\\test\\");

    const int pathSize = path.size();
    QCOMPARE(pathSize, 8);

    const int entrySize = FortCommon::logHeaderSize()
            + pathSize * sizeof(wchar_t);

    LogBuffer buf(entrySize);

    const quint32 ip = 1, pid = 2;
    LogEntry entry(ip, pid, path);

    const int testCount = 3;

    // Write
    for (int n = testCount; --n >= 0; ) {
        QCOMPARE(buf.write(entry), entrySize);
    }

    // Read
    for (int n = testCount; --n >= 0; ) {
        QCOMPARE(buf.read(entry), entrySize);
        QCOMPARE(entry.ip(), ip);
        QCOMPARE(entry.pid(), pid);
        QCOMPARE(entry.path(), path);
    }
    QCOMPARE(buf.read(entry), 0);
}
