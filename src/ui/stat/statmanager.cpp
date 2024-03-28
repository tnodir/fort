#include "statmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbutil.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
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

constexpr int DATABASE_USER_VERSION = 7;

constexpr qint32 ACTIVE_PERIOD_CHECK_SECS = 60 * OS_TICKS_PER_SECOND;

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

const IniOptions *StatManager::ini() const
{
    return &conf()->ini();
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

    m_isActivePeriodSet = false;

    if (conf()) {
        setupActivePeriod();
    }
}

void StatManager::setupActivePeriod()
{
    DateUtil::parseTime(
            conf()->activePeriodFrom(), m_activePeriodFromHour, m_activePeriodFromMinute);

    DateUtil::parseTime(conf()->activePeriodTo(), m_activePeriodToHour, m_activePeriodToMinute);
}

void StatManager::updateActivePeriod()
{
    const qint32 currentTick = OsUtil::getTickCount();

    if (!m_isActivePeriodSet || qAbs(currentTick - m_tick) >= ACTIVE_PERIOD_CHECK_SECS) {
        m_tick = currentTick;

        m_isActivePeriodSet = true;
        m_isActivePeriod = true;

        if (conf() && conf()->activePeriodEnabled()) {
            const QTime now = QTime::currentTime();

            m_isActivePeriod = DriverCommon::isTimeInPeriod(quint8(now.hour()),
                    quint8(now.minute()), m_activePeriodFromHour, m_activePeriodFromMinute,
                    m_activePeriodToHour, m_activePeriodToMinute);
        }
    }
}

void StatManager::clearQuotas(bool isNewDay, bool isNewMonth)
{
    auto quotaManager = IoC<QuotaManager>();

    quotaManager->clear(isNewDay && m_trafDay != 0, isNewMonth && m_trafMonth != 0);
}

void StatManager::checkQuotas(quint32 inBytes)
{
    if (m_isActivePeriod) {
        auto quotaManager = IoC<QuotaManager>();

        // Update quota traffic bytes
        quotaManager->addTraf(inBytes);

        quotaManager->checkQuotaDay(m_trafDay);
        quotaManager->checkQuotaMonth(m_trafMonth);
    }
}

bool StatManager::updateTrafDay(qint64 unixTime)
{
    const qint32 trafHour = DateUtil::getUnixHour(unixTime);
    const bool isNewHour = (trafHour != m_trafHour);

    const qint32 trafDay = isNewHour ? DateUtil::getUnixDay(unixTime) : m_trafDay;
    const bool isNewDay = (trafDay != m_trafDay);

    const qint32 trafMonth =
            isNewDay ? DateUtil::getUnixMonth(unixTime, ini()->monthStart()) : m_trafMonth;
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
    bool ok = true;

    beginTransaction();
    ok = sqliteDb()->execute(StatSql::sqlDeleteAllTraffic);
    commitTransaction(ok);

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

    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/stat/migrations/traf",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .migrateFunc = &migrateFunc,
    };

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

void StatManager::logClearApp(quint32 pid)
{
    m_appPidPathMap.remove(pid);
}

void StatManager::addCachedAppId(const QString &appPath, qint64 appId)
{
    m_appPathIdCache.insert(appPath, appId);
}

qint64 StatManager::getCachedAppId(const QString &appPath) const
{
    return m_appPathIdCache.value(appPath, INVALID_APP_ID);
}

void StatManager::clearCachedAppId(const QString &appPath)
{
    m_appPathIdCache.remove(appPath);
}

void StatManager::clearAppIdCache()
{
    m_appPathIdCache.clear();
}

bool StatManager::logProcNew(const LogEntryProcNew &entry, qint64 unixTime)
{
    const quint32 pid = entry.pid();
    const QString appPath = entry.path();

    Q_ASSERT(!m_appPidPathMap.contains(pid));
    m_appPidPathMap.insert(pid, appPath);

    return getOrCreateAppId(appPath, unixTime) != INVALID_APP_ID;
}

bool StatManager::logStatTraf(const LogEntryStatTraf &entry, qint64 unixTime)
{
    // Active period
    updateActivePeriod();

    const bool logStat = conf() && conf()->logStat() && m_isActivePeriod;

    const bool isNewDay = updateTrafDay(unixTime);

    sqliteDb()->beginWriteTransaction();

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

        const SqliteStmtList insertTrafAppStmts = SqliteStmtList()
                << getTrafficStmt(StatSql::sqlInsertTrafAppHour, m_trafHour)
                << getTrafficStmt(StatSql::sqlInsertTrafAppDay, m_trafDay)
                << getTrafficStmt(StatSql::sqlInsertTrafAppMonth, m_trafMonth)
                << getTrafficStmt(StatSql::sqlInsertTrafAppTotal, m_trafHour);

        const SqliteStmtList updateTrafAppStmts = SqliteStmtList()
                << getTrafficStmt(StatSql::sqlUpdateTrafAppHour, m_trafHour)
                << getTrafficStmt(StatSql::sqlUpdateTrafAppDay, m_trafDay)
                << getTrafficStmt(StatSql::sqlUpdateTrafAppMonth, m_trafMonth)
                << getTrafficStmt(StatSql::sqlUpdateTrafAppTotal, -1);

        for (int i = 0; i < procCount; ++i) {
            const quint32 pidFlag = *procTrafBytes++;
            const quint32 inBytes = *procTrafBytes++;
            const quint32 outBytes = *procTrafBytes++;

            const bool inactive = (pidFlag & 1) != 0;
            const quint32 pid = pidFlag & ~quint32(1);

            logTrafBytes(insertTrafAppStmts, updateTrafAppStmts, sumInBytes, sumOutBytes, pid,
                    inBytes, outBytes, unixTime, logStat);

            if (inactive) {
                logClearApp(pid);
            }
        }
    }

    if (logStat) {
        const SqliteStmtList insertTrafStmts = SqliteStmtList()
                << getTrafficStmt(StatSql::sqlInsertTrafHour, m_trafHour)
                << getTrafficStmt(StatSql::sqlInsertTrafDay, m_trafDay)
                << getTrafficStmt(StatSql::sqlInsertTrafMonth, m_trafMonth);

        const SqliteStmtList updateTrafStmts = SqliteStmtList()
                << getTrafficStmt(StatSql::sqlUpdateTrafHour, m_trafHour)
                << getTrafficStmt(StatSql::sqlUpdateTrafDay, m_trafDay)
                << getTrafficStmt(StatSql::sqlUpdateTrafMonth, m_trafMonth);

        // Update or insert total bytes
        updateTrafficList(insertTrafStmts, updateTrafStmts, sumInBytes, sumOutBytes);
    }

    sqliteDb()->commitTransaction();

    // Check quotas
    checkQuotas(sumInBytes);

    // Notify about sum traffic bytes
    emit trafficAdded(unixTime, sumInBytes, sumOutBytes);

    return true;
}

bool StatManager::deleteStatApp(qint64 appId)
{
    sqliteDb()->beginWriteTransaction();

    DbUtil::doList({ getIdStmt(StatSql::sqlDeleteAppTrafHour, appId),
            getIdStmt(StatSql::sqlDeleteAppTrafDay, appId),
            getIdStmt(StatSql::sqlDeleteAppTrafMonth, appId),
            getIdStmt(StatSql::sqlDeleteAppTrafTotal, appId) });

    deleteAppId(appId);

    sqliteDb()->commitTransaction();

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

bool StatManager::hasAppTraf(qint64 appId)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlSelectStatAppExists);

    stmt->bindInt64(1, appId);
    const bool res = (stmt->step() == SqliteStmt::StepRow);
    stmt->reset();

    return res;
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

qint64 StatManager::createAppId(const QString &appPath, qint64 unixTime)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlInsertAppId);

    stmt->bindText(1, appPath);
    stmt->bindInt64(2, unixTime);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return INVALID_APP_ID;
}

qint64 StatManager::getOrCreateAppId(const QString &appPath, qint64 unixTime)
{
    qint64 appId = getCachedAppId(appPath);
    if (appId == INVALID_APP_ID) {
        appId = getAppId(appPath);
        if (appId == INVALID_APP_ID) {
            if (unixTime == 0) {
                unixTime = DateUtil::getUnixTime();
            }
            appId = createAppId(appPath, unixTime);
        }

        Q_ASSERT(appId != INVALID_APP_ID);

        addCachedAppId(appPath, appId);
    }
    return appId;
}

bool StatManager::deleteAppId(qint64 appId)
{
    SqliteStmt *stmt = getIdStmt(StatSql::sqlDeleteAppId, appId);

    const bool ok = (stmt->step() == SqliteStmt::StepDone && sqliteDb()->changes() != 0);
    if (ok) {
        const QString appPath = stmt->columnText(0);
        clearCachedAppId(appPath);
    }
    stmt->reset();
    return ok;
}

void StatManager::deleteOldTraffic(qint32 trafHour)
{
    SqliteStmtList deleteTrafStmts;

    // Traffic Hour
    const int trafHourKeepDays = ini()->trafHourKeepDays();
    if (trafHourKeepDays >= 0) {
        const qint32 oldTrafHour = trafHour - 24 * trafHourKeepDays;

        deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppHour, oldTrafHour)
                        << getTrafficStmt(StatSql::sqlDeleteTrafHour, oldTrafHour);
    }

    // Traffic Day
    const int trafDayKeepDays = ini()->trafDayKeepDays();
    if (trafDayKeepDays >= 0) {
        const qint32 oldTrafDay = trafHour - 24 * trafDayKeepDays;

        deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppDay, oldTrafDay)
                        << getTrafficStmt(StatSql::sqlDeleteTrafDay, oldTrafDay);
    }

    // Traffic Month
    const int trafMonthKeepMonths = ini()->trafMonthKeepMonths();
    if (trafMonthKeepMonths >= 0) {
        const qint32 oldTrafMonth = DateUtil::addUnixMonths(trafHour, -trafMonthKeepMonths);

        deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppMonth, oldTrafMonth)
                        << getTrafficStmt(StatSql::sqlDeleteTrafMonth, oldTrafMonth);
    }

    DbUtil::doList(deleteTrafStmts);
}

void StatManager::getStatAppList(QStringList &list, QVector<qint64> &appIds)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlSelectStatAppList);

    while (stmt->step() == SqliteStmt::StepRow) {
        appIds.append(stmt->columnInt64(0));
        list.append(stmt->columnText(1));
    }
    stmt->reset();
}

void StatManager::logTrafBytes(const SqliteStmtList &insertStmtList,
        const SqliteStmtList &updateStmtList, quint32 &sumInBytes, quint32 &sumOutBytes,
        quint32 pid, quint32 inBytes, quint32 outBytes, qint64 unixTime, bool logStat)
{
    const QString appPath = m_appPidPathMap.value(pid);

    if (Q_UNLIKELY(appPath.isEmpty())) {
        qCCritical(LC) << "UI & Driver's states mismatch! Expected processes:"
                       << m_appPidPathMap.keys() << "Got:" << pid;
        return;
    }

    if (inBytes == 0 && outBytes == 0)
        return;

    const qint64 appId = getOrCreateAppId(appPath, unixTime);
    Q_ASSERT(appId != INVALID_APP_ID);

    if (logStat) {
        if (!hasAppTraf(appId)) {
            emit appCreated(appId, appPath);
        }

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

bool StatManager::beginTransaction()
{
    return sqliteDb()->beginWriteTransaction();
}

void StatManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
