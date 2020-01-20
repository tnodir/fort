#include "test.h"

#include "commontest.h"
#include "util/fileutil.h"
#include "util/processinfo.h"

void Test::paths()
{
    const QString driveC("C:");
    const QString kernelNameC("\\Device\\HarddiskVolume1");
    const QString subPathBack("\\test\\");
    const QString subPath = QString(subPathBack)
            .replace(QLatin1Char('\\'), QLatin1Char('/'));

    const QString kernelPath = kernelNameC + subPathBack;
    const QString pathBack = driveC + subPathBack;
    const QString path = driveC + subPath;

    QCOMPARE(FileUtil::kernelNameToDrive(kernelNameC), driveC);
    QCOMPARE(FileUtil::driveToKernelName(driveC), kernelNameC);

    QCOMPARE(FileUtil::kernelPathToPath(kernelPath), pathBack);
    QCOMPARE(FileUtil::pathToKernelPath(path, false), kernelPath);
}

void Test::process()
{
    const ProcessInfo pi(ProcessInfo::currentPid());

    QVERIFY(pi.isValid());
    QVERIFY(!pi.path().isEmpty());
}
