#include "test.h"

#include <QElapsedTimer>

#include "commontest.h"
#include "db/databasemanager.h"
#include "db/quotamanager.h"
#include "db/sqlite/sqlitedb.h"
#include "db/sqlite/sqlitestmt.h"
#include "fortsettings.h"
#include "util/dateutil.h"
#include "util/fileutil.h"

void Test::dbWriteRead()
{
    QStringList args("test");
    FortSettings fortSettings(args);
    QuotaManager quotaManager(&fortSettings);
    DatabaseManager databaseManager(":memory:", &quotaManager);

    QVERIFY(databaseManager.initialize());

    const QStringList appPaths = QStringList()
            << "C:\test\test.exe"
            << "C:\test\test2.exe"
            << "C:\test\test3.exe";

    const quint16 procCount = 3;
    QCOMPARE(int(procCount), appPaths.size());

    // Add apps
    quint32 index = 0;
    foreach (const QString &appPath, appPaths) {
        databaseManager.logProcNew(++index * 10, appPath);
    }

    debugProcNew(databaseManager.sqliteDb());

    QElapsedTimer timer;
    timer.start();

    // Add app traffics
    {
        const quint32 trafBytes[procCount * 3] = {
            10, 100, 200,
            20, 300, 400,
            30, 500, 600
        };

        databaseManager.logStatTraf(procCount, trafBytes);
        databaseManager.logStatTraf(procCount, trafBytes);
    }

    qDebug() << "elapsed>" << timer.restart() << "msec";

    // Delete apps
    {
        const quint32 trafBytes[procCount * 3] = {
            11, 10, 20,
            21, 30, 40,
            31, 50, 60
        };

        databaseManager.logStatTraf(procCount, trafBytes);
    }

    qDebug() << "elapsed>" << timer.elapsed() << "msec";

    debugStatTraf(databaseManager.sqliteDb());
}

void Test::debugProcNew(SqliteDb *sqliteDb)
{
    SqliteStmt stmt;

    QVERIFY(stmt.prepare(sqliteDb->db(),
                         "SELECT app_id, path FROM app;"));

    qDebug() << "> app <";
    while (stmt.step() == SqliteStmt::StepRow) {
        qDebug() << ">"
                 << stmt.columnInt64(0)
                 << stmt.columnText(1);
    }
    qDebug() << "--";
}

void Test::debugStatTraf(SqliteDb *sqliteDb)
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
                      "  FROM app;");
}

void Test::debugStatTrafStep(SqliteDb *sqliteDb, const char *name,
                             const char *sql)
{
    SqliteStmt stmt;

    QVERIFY(stmt.prepare(sqliteDb->db(), sql));

    qDebug() << '>' << name << '<';
    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 trafTime = stmt.columnInt64(1) * 3600;

        qDebug() << '>'
                 << stmt.columnInt64(0)
                 << QDateTime::fromSecsSinceEpoch(trafTime).toString()
                 << stmt.columnInt64(2)
                 << stmt.columnInt64(3);
    }
    qDebug() << "--";
}

void Test::activePeriod()
{
    const QDate d1(2018, 8, 31);
    const QTime t1(15, 35, 24);
    const QDateTime dt1(d1, t1);

    const qint64 unixTime = dt1.toSecsSinceEpoch();
    const qint32 unixDay = DateUtil::getUnixDay(unixTime);
    const qint32 unixHour = DateUtil::getUnixHour(unixTime);

    QVERIFY(DateUtil::isHourBetween(unixHour, unixDay, 0, 24));
    QVERIFY(DateUtil::isHourBetween(unixHour, unixDay, 15, 16));
    QVERIFY(DateUtil::isHourBetween(unixHour, unixDay, 15, 10));
    QVERIFY(!DateUtil::isHourBetween(unixHour, unixDay, 15, 15));
    QVERIFY(!DateUtil::isHourBetween(unixHour, unixDay, 0, 15));
    QVERIFY(!DateUtil::isHourBetween(unixHour, unixDay, 16, 15));
    QVERIFY(!DateUtil::isHourBetween(unixHour, unixDay, 24, 0));
    QVERIFY(!DateUtil::isHourBetween(unixHour, unixDay, 16, 14));
    QVERIFY(!DateUtil::isHourBetween(unixHour, unixDay, 16, 24));
}

void Test::monthStart()
{
    const QDate d1(2018, 1, 8);
    const QDateTime dt1(d1);

    const qint32 unixHour = DateUtil::getUnixMonth(dt1.toSecsSinceEpoch(), 10);

    const QDateTime dt2 = QDateTime::fromSecsSinceEpoch(
                DateUtil::toUnixTime(unixHour));
    const QDate d2 = dt2.date();

    QCOMPARE(d2.year(), 2017);
    QCOMPARE(d2.month(), 12);
    QCOMPARE(d2.day(), 1);
}
