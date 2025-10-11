#include "statmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbutil.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
#include <fortsettings.h>
#include <log/logentryprocnew.h>
#include <log/logentrystattraf.h>
#include <stat/quotamanager.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

#include "statsql.h"

namespace {

const QLoggingCategory LC("stat");

constexpr int DATABASE_USER_VERSION = 10;

constexpr qint64 INVALID_APP_ID = Q_INT64_C(-1);

bool migrateFunc(SqliteDb *db, int version, bool isNewDb, void *ctx)
{
    Q_UNUSED(ctx);

    if (isNewDb) {
        // COMPAT: DB schema
        return true;
    }

    // COMPAT: DB content
    if (version < 3) {
        // Move apps' total traffic to separate table
        const QString srcSchema = SqliteDb::migrationOldSchemaName();
        const QString dstSchema = SqliteDb::migrationNewSchemaName();

        const auto sql = QString("INSERT INTO %1 (%3) SELECT %3 FROM %2;")
                                 .arg(SqliteDb::entityName(dstSchema, "traffic_app"),
                                         SqliteDb::entityName(srcSchema, "app"),
                                         "app_id, traf_time, in_bytes, out_bytes");
        db->executeStr(sql);
    }

    return true;
}

SqliteDb::MigrateOptions migrateOptions()
{
    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/stat/migrations/traf",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .migrateFunc = &migrateFunc,
        .ftsTables = {
            {
                .contentTable = "app",
                .contentRowid = "app_id",
                .columns = { "path", "name" },
            },
        },
    };

    return opt;
}

}

StatManager::StatManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    QObject(parent), m_sqliteDb(new SqliteDb(filePath, openFlags))
{
}

void StatManager::setConf(const FirewallConf *conf)
{
    m_conf = conf;

    setupByConf();
}

IniOptions &StatManager::ini() const
{
    return IoC<FortSettings>()->iniOpt();
}

void StatManager::setUp()
{
    setupDb();
}

void StatManager::setupTrafDate()
{
    m_trafHour = m_trafDay = m_trafMonth = 0;
}

void StatManager::setupByConf()
{
    if (!conf()) {
        logClear();
    }

    m_tickSecs = 0;

    if (conf()) {
        setupActivePeriod();
    }
}

void StatManager::setupActivePeriod()
{
    m_activePeriodFrom = DateUtil::parseTime(conf()->activePeriodFrom());

    m_activePeriodTo = DateUtil::parseTime(conf()->activePeriodTo());
}

void StatManager::updateActivePeriod(qint32 tickSecs)
{
    constexpr qint32 ACTIVE_PERIOD_CHECK_SECS = 60;

    if (qAbs(tickSecs - m_tickSecs) < ACTIVE_PERIOD_CHECK_SECS)
        return;

    m_tickSecs = tickSecs;

    m_isActivePeriod = true;

    if (conf() && conf()->activePeriodEnabled()) {
        const QTime now = DateUtil::currentTime();

        m_isActivePeriod = DateUtil::isTimeInPeriod(now, m_activePeriodFrom, m_activePeriodTo);
    }
}

void StatManager::clearQuotas(bool isNewDay, bool isNewMonth)
{
    auto quotaManager = IoC<QuotaManager>();

    quotaManager->clear(isNewDay && m_trafDay != 0, isNewMonth && m_trafMonth != 0);
}

void StatManager::checkQuotas(quint32 inBytes)
{
    if (!m_isActivePeriod)
        return;

    auto quotaManager = IoC<QuotaManager>();

    // Update quota traffic bytes
    quotaManager->addTraf(inBytes);

    quotaManager->checkQuotaDay(m_trafDay);
    quotaManager->checkQuotaMonth(m_trafMonth);
}

bool StatManager::updateTrafDay(qint64 unixTime)
{
    const qint32 trafHour = DateUtil::getUnixHour(unixTime);
    const bool isNewHour = (trafHour != m_trafHour);

    const qint32 trafDay = isNewHour ? DateUtil::getUnixDay(unixTime) : m_trafDay;
    const bool isNewDay = (trafDay != m_trafDay);

    const qint32 trafMonth =
            isNewDay ? DateUtil::getUnixMonth(unixTime, ini().monthStart()) : m_trafMonth;
    const bool isNewMonth = (trafMonth != m_trafMonth);

    // Initialize quotas traffic bytes
    clearQuotas(isNewDay, isNewMonth);

    m_trafHour = trafHour;
    m_trafDay = trafDay;
    m_trafMonth = trafMonth;

    return isNewDay;
}

bool StatManager::clearTraffic()
{
    bool ok;

    beginWriteTransaction();

    ok = sqliteDb()->execute(StatSql::sqlDeleteAllTraffic);

    endTransaction(ok);

    if (!ok)
        return false;

    sqliteDb()->vacuum(); // Vacuum outside of transaction

    clearAppIdCache();

    setupTrafDate();

    IoC<QuotaManager>()->clear();

    emit trafficCleared();

    return true;
}

bool StatManager::setupDb()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return false;
    }

    SqliteDb::MigrateOptions opt = migrateOptions();

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return false;
    }

    return true;
}

void StatManager::logClear()
{
    m_appPidPathMap.clear();
}

void StatManager::addLoggedProcessId(const QString &appPath, const LogEntryProcNew &entry)
{
    const quint32 pid = entry.pid();

    Q_ASSERT(!m_appPidPathMap.contains(pid));

    m_appPidPathMap.insert(pid, appPath);
}

void StatManager::removeLoggedProcessId(quint32 pid)
{
    m_appPidPathMap.remove(pid);
}

QString StatManager::getLoggedProcessIdPath(quint32 pid)
{
    return m_appPidPathMap.value(pid);
}

void StatManager::addCachedAppId(const QString &appPath, qint64 appId)
{
    m_appPathIdCache.insert(appPath, appId);
}

qint64 StatManager::getCachedAppId(const QString &appPath) const
{
    return m_appPathIdCache.value(appPath, INVALID_APP_ID);
}

void StatManager::removeCachedAppId(const QString &appPath)
{
    m_appPathIdCache.remove(appPath);
}

void StatManager::clearAppIdCache()
{
    m_appPathIdCache.clear();
}

bool StatManager::logProcNew(const LogEntryProcNew &entry, qint64 unixTime)
{
    const QString appPath = entry.path();

    addLoggedProcessId(appPath, entry);

    return getOrCreateCachedAppId(appPath, entry.appId(), unixTime) != INVALID_APP_ID;
}

bool StatManager::logStatTraf(const LogEntryStatTraf &entry, qint64 unixTime)
{
    // Active period
    updateActivePeriod(qint32(unixTime));

    const bool logStat = conf() && conf()->logStat() && m_isActivePeriod;

    const bool isNewDay = updateTrafDay(unixTime);

    beginWriteTransaction();

    // Delete old data
    if (isNewDay) {
        deleteOldTraffic(m_trafHour);
    }

    // Sum traffic bytes
    quint32 sumInBytes = 0;
    quint32 sumOutBytes = 0;

    const quint16 procCount = entry.procCount();
    {
        const quint32 *procTrafBytes = entry.procTrafBytes();

        const SqliteStmtList insertTrafAppStmts = {
            getTrafficStmt(StatSql::sqlInsertTrafAppHour, m_trafHour),
            getTrafficStmt(StatSql::sqlInsertTrafAppDay, m_trafDay),
            getTrafficStmt(StatSql::sqlInsertTrafAppMonth, m_trafMonth),
            getTrafficStmt(StatSql::sqlInsertTrafAppTotal, m_trafHour),
        };

        const SqliteStmtList updateTrafAppStmts = {
            getTrafficStmt(StatSql::sqlUpdateTrafAppHour, m_trafHour),
            getTrafficStmt(StatSql::sqlUpdateTrafAppDay, m_trafDay),
            getTrafficStmt(StatSql::sqlUpdateTrafAppMonth, m_trafMonth),
            getTrafficStmt(StatSql::sqlUpdateTrafAppTotal, -1),
        };

        for (int i = 0; i < procCount; ++i) {
            const quint32 pidFlag = *procTrafBytes++;
            const quint32 inBytes = *procTrafBytes++;
            const quint32 outBytes = *procTrafBytes++;

            const bool inactive = (pidFlag & 1) != 0;
            const quint32 pid = pidFlag & ~quint32(1);

            logTrafBytes(insertTrafAppStmts, updateTrafAppStmts, sumInBytes, sumOutBytes, pid,
                    inBytes, outBytes, unixTime, logStat);

            if (inactive) {
                removeLoggedProcessId(pid);
            }
        }
    }

    if (logStat) {
        const SqliteStmtList insertTrafStmts = {
            getTrafficStmt(StatSql::sqlInsertTrafHour, m_trafHour),
            getTrafficStmt(StatSql::sqlInsertTrafDay, m_trafDay),
            getTrafficStmt(StatSql::sqlInsertTrafMonth, m_trafMonth),
        };

        const SqliteStmtList updateTrafStmts = {
            getTrafficStmt(StatSql::sqlUpdateTrafHour, m_trafHour),
            getTrafficStmt(StatSql::sqlUpdateTrafDay, m_trafDay),
            getTrafficStmt(StatSql::sqlUpdateTrafMonth, m_trafMonth),
        };

        // Update or insert total bytes
        updateTrafficList(insertTrafStmts, updateTrafStmts, sumInBytes, sumOutBytes);
    }

    commitTransaction();

    // Check quotas
    checkQuotas(sumInBytes);

    // Notify about sum traffic bytes
    emit trafficAdded(unixTime, sumInBytes, sumOutBytes);

    return true;
}

bool StatManager::deleteStatApp(qint64 appId)
{
    beginWriteTransaction();

    DbUtil::doList({ getIdStmt(StatSql::sqlDeleteAppTrafHour, appId),
            getIdStmt(StatSql::sqlDeleteAppTrafDay, appId),
            getIdStmt(StatSql::sqlDeleteAppTrafMonth, appId),
            getIdStmt(StatSql::sqlDeleteAppTrafTotal, appId) });

    deleteAppId(appId);

    commitTransaction();

    emit appStatRemoved(appId);

    return true;
}

bool StatManager::resetAppTrafTotals()
{
    SqliteStmt *stmt = getStmt(StatSql::sqlResetAppTrafTotals);
    const qint64 unixTime = DateUtil::getUnixTime();

    stmt->bindInt(1, DateUtil::getUnixHour(unixTime));

    const bool ok = sqliteDb()->done(stmt);

    if (ok) {
        emit appTrafTotalsResetted();
    }

    return ok;
}

qint64 StatManager::getAppId(const QString &appPath)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = getStmt(StatSql::sqlSelectAppId);

    stmt->bindText(1, appPath);
    if (stmt->step() == SqliteStmt::StepRow) {
        appId = stmt->columnInt64();
    }
    stmt->reset();

    return appId;
}

qint64 StatManager::createAppId(const QString &appPath, quint32 confAppId, qint64 unixTime)
{
    const auto appName = IoC<AppInfoCache>()->appName(appPath);

    SqliteStmt *stmt = getStmt(StatSql::sqlInsertAppId);

    stmt->bindVar(1, DbVar::nullable(confAppId));
    stmt->bindText(2, appPath);
    stmt->bindText(3, appName);
    stmt->bindInt64(4, unixTime);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return INVALID_APP_ID;
}

qint64 StatManager::getOrCreateAppId(const QString &appPath, quint32 confAppId, qint64 unixTime)
{
    qint64 appId = getAppId(appPath);
    if (appId == INVALID_APP_ID) {
        if (unixTime == 0) {
            unixTime = DateUtil::getUnixTime();
        }
        appId = createAppId(appPath, confAppId, unixTime);

        Q_ASSERT(appId != INVALID_APP_ID);

        emit appCreated(appId, appPath);
    }
    return appId;
}

qint64 StatManager::getOrCreateCachedAppId(
        const QString &appPath, quint32 confAppId, qint64 unixTime)
{
    qint64 appId = getCachedAppId(appPath);
    if (appId == INVALID_APP_ID) {
        appId = getOrCreateAppId(appPath, confAppId, unixTime);

        addCachedAppId(appPath, appId);
    }
    return appId;
}

bool StatManager::deleteAppId(qint64 appId)
{
    SqliteStmt *stmt = getIdStmt(StatSql::sqlDeleteAppId, appId);

    const auto stepRes = stmt->step();
    const bool ok = (stepRes != SqliteStmt::StepError && sqliteDb()->changes() != 0);

    if (ok) {
        const QString appPath = stmt->columnText(0);
        removeCachedAppId(appPath);
    }

    stmt->reset();

    return ok;
}

void StatManager::deleteOldTraffic(qint32 trafHour)
{
    SqliteStmtList deleteTrafStmts;

    // Traffic Hour
    const int trafHourKeepDays = ini().trafHourKeepDays();
    if (trafHourKeepDays >= 0) {
        const qint32 oldTrafHour = trafHour - 24 * trafHourKeepDays;

        deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppHour, oldTrafHour)
                        << getTrafficStmt(StatSql::sqlDeleteTrafHour, oldTrafHour);
    }

    // Traffic Day
    const int trafDayKeepDays = ini().trafDayKeepDays();
    if (trafDayKeepDays >= 0) {
        const qint32 oldTrafDay = trafHour - 24 * trafDayKeepDays;

        deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppDay, oldTrafDay)
                        << getTrafficStmt(StatSql::sqlDeleteTrafDay, oldTrafDay);
    }

    // Traffic Month
    const int trafMonthKeepMonths = ini().trafMonthKeepMonths();
    if (trafMonthKeepMonths >= 0) {
        const qint32 oldTrafMonth = DateUtil::addUnixMonths(trafHour, -trafMonthKeepMonths);

        deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppMonth, oldTrafMonth)
                        << getTrafficStmt(StatSql::sqlDeleteTrafMonth, oldTrafMonth);
    }

    DbUtil::doList(deleteTrafStmts);
}

void StatManager::logTrafBytes(const SqliteStmtList &insertStmtList,
        const SqliteStmtList &updateStmtList, quint32 &sumInBytes, quint32 &sumOutBytes,
        quint32 pid, quint32 inBytes, quint32 outBytes, qint64 unixTime, bool logStat)
{
    const QString appPath = getLoggedProcessIdPath(pid);

    if (Q_UNLIKELY(appPath.isEmpty())) {
        qCCritical(LC) << "UI & Driver's states mismatch! Expected processes:"
                       << m_appPidPathMap.keys() << "Got:" << pid;
        return;
    }

    if (inBytes == 0 && outBytes == 0)
        return;

    const qint64 appId = getOrCreateCachedAppId(appPath, /*confAppId=*/0, unixTime);
    Q_ASSERT(appId != INVALID_APP_ID);

    if (logStat) {
        // Update or insert app bytes
        updateTrafficList(insertStmtList, updateStmtList, inBytes, outBytes, appId);
    }

    // Update sum traffic bytes
    sumInBytes += inBytes;
    sumOutBytes += outBytes;
}

void StatManager::updateTrafficList(const SqliteStmtList &insertStmtList,
        const SqliteStmtList &updateStmtList, quint32 inBytes, quint32 outBytes, qint64 appId)
{
    int i = 0;
    for (SqliteStmt *stmtUpdate : updateStmtList) {
        if (!updateTraffic(stmtUpdate, inBytes, outBytes, appId)) {
            SqliteStmt *stmtInsert = insertStmtList.at(i);
            if (!updateTraffic(stmtInsert, inBytes, outBytes, appId)) {
                qCCritical(LC) << "Update traffic error:" << sqliteDb()->errorMessage()
                               << "inBytes:" << inBytes << "outBytes:" << outBytes
                               << "appId:" << appId << "index:" << i;
            }
        }
        ++i;
    }
}

bool StatManager::updateTraffic(SqliteStmt *stmt, quint32 inBytes, quint32 outBytes, qint64 appId)
{
    stmt->bindInt64(2, inBytes);
    stmt->bindInt64(3, outBytes);

    if (appId != 0) {
        stmt->bindInt64(4, appId);
    }

    return sqliteDb()->done(stmt);
}

qint32 StatManager::getTrafficTime(const char *sql, qint64 appId)
{
    qint32 trafTime = 0;

    SqliteStmt *stmt = getStmt(sql);

    if (appId != 0) {
        stmt->bindInt64(1, appId);
    }

    if (stmt->step() == SqliteStmt::StepRow) {
        trafTime = stmt->columnInt();
    }
    stmt->reset();

    return trafTime;
}

void StatManager::getTraffic(
        const char *sql, qint32 trafTime, qint64 &inBytes, qint64 &outBytes, qint64 appId)
{
    SqliteStmt *stmt = getStmt(sql);

    stmt->bindInt(1, trafTime);

    if (appId != 0) {
        stmt->bindInt64(2, appId);
    }

    if (stmt->step() == SqliteStmt::StepRow) {
        inBytes = stmt->columnInt64(0);
        outBytes = stmt->columnInt64(1);
    } else {
        inBytes = outBytes = 0;
    }

    stmt->reset();
}

bool StatManager::exportBackup(const QString &path)
{
    FileUtil::makePath(path);

    const QString outPath = FileUtil::pathSlash(path);

    // Export DB
    if (!exportMasterBackup(outPath)) {
        qCWarning(LC) << "Export error:" << path;
        return false;
    }

    return true;
}

bool StatManager::exportMasterBackup(const QString &path)
{
    // Export Db
    if (!backupDbFile(path)) {
        qCWarning(LC) << "Export Db error:" << sqliteDb()->errorMessage();
        return false;
    }

    return true;
}

bool StatManager::importBackup(const QString &path)
{
    const QString inPath = FileUtil::pathSlash(path);

    // Import DB
    if (!importMasterBackup(inPath)) {
        qCWarning(LC) << "Import error:" << path;
        return false;
    }

    return true;
}

bool StatManager::importMasterBackup(const QString &path)
{
    // Import Db
    SqliteDb::MigrateOptions opt = migrateOptions();

    opt.backupFilePath = path + FileUtil::fileName(sqliteDb()->filePath());

    if (!sqliteDb()->import(opt))
        return false;

    return true;
}

SqliteStmt *StatManager::getStmt(const char *sql)
{
    return sqliteDb()->stmt(sql);
}

SqliteStmt *StatManager::getTrafficStmt(const char *sql, qint32 trafTime)
{
    SqliteStmt *stmt = getStmt(sql);

    stmt->bindInt(1, trafTime);

    return stmt;
}

SqliteStmt *StatManager::getIdStmt(const char *sql, qint64 id)
{
    SqliteStmt *stmt = getStmt(sql);

    stmt->bindInt64(1, id);

    return stmt;
}
