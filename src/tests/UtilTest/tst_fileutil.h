#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <util/fileutil.h>
#include <util/processinfo.h>

class FileUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void FileUtilTest::SetUp() { }

void FileUtilTest::TearDown() { }

TEST_F(FileUtilTest, paths)
{
    const QString driveC("C:");

    const QString kernelNameC = FileUtil::driveToKernelName(driveC);
    ASSERT_TRUE(kernelNameC.startsWith("\\Device\\HarddiskVolume", Qt::CaseSensitive));

    ASSERT_EQ(FileUtil::kernelNameToDrive(kernelNameC), driveC);

    const QString subPathBack("\\test\\");
    const QString subPath = QString(subPathBack).replace(QLatin1Char('\\'), QLatin1Char('/'));

    const QString kernelPath = kernelNameC + subPathBack;
    const QString pathBack = driveC + subPathBack;
    const QString path = driveC + subPath;

    ASSERT_EQ(FileUtil::kernelPathToPath(kernelPath), pathBack);
    ASSERT_EQ(FileUtil::pathToKernelPath(path, /*lower=*/false), kernelPath);
}

TEST_F(FileUtilTest, mupPath)
{
    const QString path(R"(\device\mup\vmware-host\shared folders\d\test.exe)");

    ASSERT_EQ(FileUtil::kernelPathToPath(path), path);
}

TEST_F(FileUtilTest, systemPath)
{
    ASSERT_EQ(FileUtil::pathToKernelPath("System", /*lower=*/true), FileUtil::systemApp());
}

TEST_F(FileUtilTest, process)
{
    const ProcessInfo pi(ProcessInfo::currentPid());

    ASSERT_TRUE(pi.isValid());
    ASSERT_FALSE(pi.path().isEmpty());
}
