#include "test.h"

#include "commontest.h"
#include "fortcommon.h"
#include "log/logbuffer.h"
#include "log/logentryblocked.h"

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
    LogEntryBlocked entry(ip, pid, path);

    // Write
    for (int i = 0; i < testCount; ++i) {
        buf.writeEntryBlocked(&entry);
    }

    // Read
    int readCount = 0;
    while (buf.peekEntryType() == LogEntry::AppBlocked) {
        buf.readEntryBlocked(&entry);

        QCOMPARE(entry.type(), LogEntry::AppBlocked);
        QCOMPARE(entry.ip(), ip);
        QCOMPARE(entry.pid(), pid);
        QCOMPARE(entry.kernelPath(), path);

        ++readCount;
    }
    QCOMPARE(readCount, testCount);
}
