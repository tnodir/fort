#include "test.h"

#include <QtTest>

#include "util/fileutil.h"
#include "util/processinfo.h"

void Test::paths()
{
    const QString driveC("C:");
    const QString dosNameC("\\Device\\HarddiskVolume1");
    const QString subPath("\\test\\");

    const QString path = driveC + subPath;
    const QString dosPath = dosNameC + subPath;

    QCOMPARE(FileUtil::dosNameToDrive(dosNameC), driveC);
    QCOMPARE(FileUtil::driveToDosName(driveC), dosNameC);

    QCOMPARE(FileUtil::dosPathToPath(dosPath), path);
    QCOMPARE(FileUtil::pathToDosPath(path), dosPath);
}

void Test::process()
{
    const ProcessInfo pi(ProcessInfo::currentPid());

    QVERIFY(pi.isValid());
    QVERIFY(!pi.dosPath().isEmpty());
}
