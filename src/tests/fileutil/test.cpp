#include "test.h"

#include <QtTest>

#include "util/fileutil.h"
#include "util/processinfo.h"

void Test::paths()
{
    const QString driveC("C:");
    const QString dosNameC("\\Device\\HarddiskVolume1");
    const QString subPathBack("\\test\\");
    const QString subPath = QString(subPathBack)
            .replace(QLatin1Char('\\'), QLatin1Char('/'));

    const QString kernelPath = dosNameC + subPathBack;
    const QString pathBack = driveC + subPathBack;
    const QString path = driveC + subPath;

    QCOMPARE(FileUtil::dosNameToDrive(dosNameC), driveC);
    QCOMPARE(FileUtil::driveToDosName(driveC), dosNameC);

    QCOMPARE(FileUtil::kernelPathToPath(kernelPath), pathBack);
    QCOMPARE(FileUtil::pathToKernelPath(path), kernelPath);
}

void Test::process()
{
    const ProcessInfo pi(ProcessInfo::currentPid());

    QVERIFY(pi.isValid());
    QVERIFY(!pi.kernelPath().isEmpty());
}
