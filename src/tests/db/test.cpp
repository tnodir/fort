#include "test.h"

#include <QtTest>

#include "db/databasemanager.h"
#include "util/fileutil.h"

void Test::dbWriteRead()
{
    DatabaseManager databaseManager(":memory:");

    QVERIFY(databaseManager.initialize());

    const QStringList appPaths = QStringList()
            << "C:\test\test.exe"
            << "C:\test\test2.exe"
            << "C:\test\test3.exe";

    const quint16 procCount = 3;
    QCOMPARE(int(procCount), appPaths.size());

    // Add apps
    foreach (const QString &appPath, appPaths) {
        databaseManager.handleProcNew(appPath);
    }

    databaseManager.debugProcNew();

    // Add app traffics
    {
        const quint8 procBits = 0xFF;
        QVERIFY(sizeof(procBits) * 8 >= procCount);

        const quint32 trafBytes[procCount * 2] = {100, 200, 300, 400, 500, 600};

        databaseManager.handleStatTraf(procCount, &procBits, trafBytes);
        databaseManager.handleStatTraf(procCount, &procBits, trafBytes);
    }

    databaseManager.debugStatTraf();

    // Delete apps
    {
        const quint8 procBits = 0x02;
        QVERIFY(sizeof(procBits) * 8 >= procCount);

        const quint32 trafBytes[procCount * 2] = {10, 20, 30, 40, 50, 60};

        databaseManager.handleStatTraf(procCount, &procBits, trafBytes);
    }

    databaseManager.debugStatTraf();
}
