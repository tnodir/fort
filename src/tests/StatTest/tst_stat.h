#pragma once

#include <QDebug>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTemporaryDir>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <googletest.h>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/conftimeperiodmanager.h>
#include <conf/firewallconf.h>
#include <fortsettings.h>
#include <log/logentryprocnew.h>
#include <log/logentrystattraf.h>
#include <stat/quotamanager.h>
#include <stat/statmanager.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

#include <mocks/mockappinfocache.h>
#include <mocks/mockquotamanager.h>

class StatTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void StatTest::SetUp() { }

void StatTest::TearDown() { }

namespace {

void debugProcNew(SqliteDb *sqliteDb)
{
    SqliteStmt stmt;

    ASSERT_TRUE(stmt.prepare(sqliteDb, "SELECT app_id, path FROM app;"));

    qDebug() << "> app <";
    while (stmt.step() == SqliteStmt::StepRow) {
        qDebug() << ">" << stmt.columnInt64(0) << stmt.columnText(1);
    }
    qDebug() << "--";
}

void debugStatTrafStep(SqliteDb *sqliteDb, const char *name, const char *sql)
{
    SqliteStmt stmt;

    if (!stmt.prepare(sqliteDb, sql)) {
        qWarning() << "SqliteStmt prepare error" << sql << sqliteDb->errorMessage();
        Q_UNREACHABLE();
        return;
    }

    qDebug() << '>' << name << '<';
    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 trafTime = stmt.columnInt64(1) * 3600;

        qDebug() << '>' << stmt.columnInt64(0) << QDateTime::fromSecsSinceEpoch(trafTime).toString()
                 << stmt.columnInt64(2) << stmt.columnInt64(3);
    }
    qDebug() << "--";
}

void debugStatTraf(SqliteDb *sqliteDb)
{
    debugStatTrafStep(sqliteDb, "traffic_app_hour",
            "SELECT app_id, traf_time, in_bytes, out_bytes"
            "  FROM traffic_app_hour;");
    debugStatTrafStep(sqliteDb, "traffic_app_day",
            "SELECT app_id, traf_time, in_bytes, out_bytes"
            "  FROM traffic_app_day;");
    debugStatTrafStep(sqliteDb, "traffic_app_month",
            "SELECT app_id, traf_time, in_bytes, out_bytes"
            "  FROM traffic_app_month;");

    debugStatTrafStep(sqliteDb, "traffic_hour",
            "SELECT 0, traf_time, in_bytes, out_bytes"
            "  FROM traffic_hour;");
    debugStatTrafStep(sqliteDb, "traffic_day",
            "SELECT 0, traf_time, in_bytes, out_bytes"
            "  FROM traffic_day;");
    debugStatTrafStep(sqliteDb, "traffic_month",
            "SELECT 0, traf_time, in_bytes, out_bytes"
            "  FROM traffic_month;");

    debugStatTrafStep(sqliteDb, "traffic_app_total",
            "SELECT app_id, traf_time, in_bytes, out_bytes"
            "  FROM traffic_app;");
}

// Holds the WAL's read locks of the DB's -shm file, so that no reader can start: SQLite sleeps
// ~10 seconds and returns SQLITE_PROTOCOL
class WalReadLocker
{
public:
    explicit WalReadLocker(const QString &dbFilePath)
    {
        const QString shmPath = QDir::toNativeSeparators(dbFilePath + "-shm");

        m_handle = ::CreateFileW(reinterpret_cast<LPCWSTR>(shmPath.utf16()),
                GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    ~WalReadLocker()
    {
        if (m_handle != INVALID_HANDLE_VALUE) {
            ::CloseHandle(m_handle);
        }
    }

    bool lock()
    {
        // WALINDEX_LOCK_OFFSET + WAL_READ_LOCK(0..4)
        constexpr DWORD lockOffset = 120 + 3;
        constexpr DWORD readLockCount = 5;

        for (DWORD i = 0; i < readLockCount; ++i) {
            OVERLAPPED ov = {};
            ov.Offset = lockOffset + i;

            if (!::LockFileEx(m_handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1,
                        0, &ov))
                return false;
        }
        return true;
    }

private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;
};

}

TEST_F(StatTest, dbWriteRead)
{
    IocContainer ioc;
    ioc.pinToThread();

    ConfManager confManager(":memory:");
    ConfTimePeriodManager confTimePeriodManager;
    NiceMock<MockAppInfoCache> appInfoCache;
    NiceMock<MockQuotaManager> quotaManager;

    ioc.set<ConfManager>(confManager);
    ioc.set<ConfTimePeriodManager>(confTimePeriodManager);
    ioc.set<AppInfoCache>(appInfoCache);
    ioc.set<QuotaManager>(quotaManager);

    StatManager statManager(":memory:");

    statManager.setUp();

    const QStringList appPaths = QStringList() << "C:\\test\\test.exe"
                                               << "C:\\test\\test2.exe"
                                               << "C:\\test\\test3.exe";

    const quint16 procCount = 3;
    ASSERT_EQ(int(procCount), appPaths.size());

    // Add apps
    quint32 index = 0;
    for (const QString &appPath : appPaths) {
        LogEntryProcNew entry(++index * 10, appPath);
        entry.setInheritKernelPath(index == procCount ? appPaths[0] : QString());
        statManager.logProcNew(entry);
    }

    // The processes' paths by their Process New entries
    {
        const auto process = statManager.loggedProcess(procCount * 10);
        ASSERT_TRUE(process);
        ASSERT_EQ(process->kernelPath, appPaths[procCount - 1]);
        ASSERT_EQ(process->inheritKernelPath, appPaths[0]);

        ASSERT_FALSE(statManager.loggedProcess(1));
    }

    debugProcNew(statManager.sqliteDb());

    QElapsedTimer timer;
    timer.start();

    // Add app traffics
    {
        const quint32 trafBytes[procCount * 3] = { 10, 100, 200, 20, 300, 400, 30, 500, 600 };

        LogEntryStatTraf entry(procCount, trafBytes);
        statManager.logStatTraf(entry);
        statManager.logStatTraf(entry);
    }

    qDebug() << "elapsed>" << timer.restart() << "msec";

    // Delete apps
    {
        const quint32 trafBytes[procCount * 3] = { 11, 10, 20, 21, 30, 40, 31, 50, 60 };

        LogEntryStatTraf entry(procCount, trafBytes);
        statManager.logStatTraf(entry);
    }

    qDebug() << "elapsed>" << timer.elapsed() << "msec";

    debugStatTraf(statManager.sqliteDb());
}

TEST_F(StatTest, monthStart)
{
    const QDate d1(2018, 1, 8);
    const QDateTime dt1(d1, QTime());

    const qint32 unixHour = DateUtil::getUnixMonth(dt1.toSecsSinceEpoch(), 10);

    const QDateTime dt2 = QDateTime::fromSecsSinceEpoch(DateUtil::toUnixTime(unixHour));
    const QDate d2 = dt2.date();

    ASSERT_EQ(d2.year(), 2017);
    ASSERT_EQ(d2.month(), 12);
    ASSERT_EQ(d2.day(), 1);
}

TEST_F(StatTest, dbProtocolError)
{
    IocContainer ioc;
    ioc.pinToThread();

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    const QString filePath = dir.filePath("stat.db");

    ConfManager confManager(":memory:");
    ConfTimePeriodManager confTimePeriodManager;
    NiceMock<MockAppInfoCache> appInfoCache;
    NiceMock<MockQuotaManager> quotaManager;

    ioc.set<ConfManager>(confManager);
    ioc.set<ConfTimePeriodManager>(confTimePeriodManager);
    ioc.set<AppInfoCache>(appInfoCache);
    ioc.set<QuotaManager>(quotaManager);

    StatManager statManager(filePath);

    statManager.setUp();

    const QString appPath = "C:\\test\\test.exe";
    const quint32 procId = 10;
    const quint32 trafBytes[3] = { procId, 100, 200 }; // pid, in, out

    LogEntryProcNew procEntry(procId, appPath);
    ASSERT_TRUE(statManager.logProcNew(procEntry));

    LogEntryStatTraf trafEntry(1, trafBytes);
    ASSERT_TRUE(statManager.logStatTraf(trafEntry));

    SqliteDb *sqliteDb = statManager.sqliteDb();
    ASSERT_FALSE(sqliteDb->isProtocolBlocked());

    WalReadLocker locker(filePath);
    ASSERT_TRUE(locker.lock());

    QSignalSpy trafficAddedSpy(&statManager, &StatManager::trafficAdded);

    // The first statement waits for SQLite's retries, then the DB is blocked
    ASSERT_TRUE(statManager.logStatTraf(trafEntry));
    ASSERT_TRUE(sqliteDb->isProtocolBlocked());
    ASSERT_EQ(trafficAddedSpy.size(), 1);

    // The blocked DB fails at once, the traffic is still counted
    QElapsedTimer timer;
    timer.start();

    ASSERT_FALSE(statManager.logProcNew(LogEntryProcNew(procId + 1, appPath + "2")));
    ASSERT_TRUE(statManager.logStatTraf(trafEntry));
    ASSERT_FALSE(sqliteDb->execute("BEGIN IMMEDIATE;"));

    ASSERT_LT(timer.elapsed(), 1000);
    ASSERT_EQ(trafficAddedSpy.size(), 2);
}
