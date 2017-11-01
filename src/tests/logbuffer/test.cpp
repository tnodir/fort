#include "test.h"

#include <QtTest>

#include "fortcommon.h"
#include "log/logbuffer.h"
#include "log/logentry.h"

void Test::logWriteRead()
{
    const QString path("C:\\test\\");

    const int pathSize = path.size();
    QCOMPARE(pathSize, 8);

    const int entrySize = FortCommon::logBlockedHeaderSize()
            + pathSize * sizeof(wchar_t);

    const int testCount = 3;

    LogBuffer buf(entrySize * testCount);

    const quint32 ip = 1, pid = 2;
    LogEntry entry(ip, pid, path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        QCOMPARE(buf.write(&entry), entrySize);
    }

    // Read
    while (buf.read(&entry)) {
        QCOMPARE(entry.ip(), ip);
        QCOMPARE(entry.pid(), pid);
        QCOMPARE(entry.kernelPath(), path);
    }
}
