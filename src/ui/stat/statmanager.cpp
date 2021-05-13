#include "statmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../conf/firewallconf.h"
#include "../driver/drivercommon.h"
#include "../log/logentryblockedip.h"
#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "../util/osutil.h"
#include "quotamanager.h"
#include "statsql.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_STAT_MANAGER)
Q_LOGGING_CATEGORY(CLOG_STAT_MANAGER, "stat")

#define logWarning()  qCWarning(CLOG_STAT_MANAGER, )
#define logCritical() qCCritical(CLOG_STAT_MANAGER, )

#define DATABASE_USER_VERSION 4

#define ACTIVE_PERIOD_CHECK_SECS (60 * OS_TICKS_PER_SECOND)

#define INVALID_APP_ID qint64(-1)

namespace {

bool migrateFunc(SqliteDb *db, int version, bool isNewDb, void *ctx)
{
    Q_UNUSED(ctx);

    if (version == 3 && !isNewDb) {
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

StatManager::StatManager(
        const QString &filePath, QuotaManager *quotaManager, QObject *parent, quint32 openFlags) :
    QObject(parent),
    m_isActivePeriodSet(false),
    m_isActivePeriod(false),
    m_quotaManager(quotaManager),
    m_sqliteDb(new SqliteDb(filePath, openFlags))
{
}

StatManager::~StatManager()
{
    delete sqliteDb();
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

bool StatManager::initialize()
{
    if (!sqliteDb()->open()) {
        logCritical() << "File open error:" << sqliteDb()->filePath() << sqliteDb()->errorMessage();
        return false;
    }

    if (!sqliteDb()->migrate(
                ":/stat/migrations", nullptr, DATABASE_USER_VERSION, true, true, &migrateFunc)) {
        logCritical() << "Migration error" << sqliteDb()->filePath();
        return false;
    }

    setupAfterClear();

    connect(this, &StatManager::cleared, this, &StatManager::setupAfterClear);

    return true;
}

void StatManager::setupAfterClear()
{
    m_trafHour = m_trafDay = m_trafMonth = 0;

    setupConnBlockId();
}

void StatManager::setupConnBlockId()
{
    const auto vars = sqliteDb()->executeEx(StatSql::sqlSelectMinMaxConnBlockId, {}, 2).toList();
    if (vars.size() == 2) {
        m_connBlockIdMin = vars.at(0).toLongLong();
        m_connBlockIdMax = vars.at(1).toLongLong();
    }
}

void StatManager::setupByConf()
{
    if (!conf() || !conf()->logStat()) {
        logClear();
    }

    m_isActivePeriodSet = false;

    if (conf()) {
        setupActivePeriod();
        setupQuota();
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

        if (conf()->activePeriodEnabled()) {
            const QTime now = QTime::currentTime();

            m_isActivePeriod = DriverCommon::isTimeInPeriod(quint8(now.hour()),
                    quint8(now.minute()), m_activePeriodFromHour, m_activePeriodFromMinute,
                    m_activePeriodToHour, m_activePeriodToMinute);
        }
    }
}

void StatManager::setupQuota()
{
    quotaManager()->setQuotaDayBytes(qint64(ini()->quotaDayMb()) * 1024 * 1024);
    quotaManager()->setQuotaMonthBytes(qint64(ini()->quotaMonthMb()) * 1024 * 1024);

    const qint64 unixTime = DateUtil::getUnixTime();
    const qint32 trafDay = DateUtil::getUnixDay(unixTime);
    const qint32 trafMonth = DateUtil::getUnixMonth(unixTime, ini()->monthStart());

    qint64 inBytes, outBytes;

    getTraffic(StatSql::sqlSelectTrafDay, trafDay, inBytes, outBytes);
    quotaManager()->setTrafDayBytes(inBytes);

    getTraffic(StatSql::sqlSelectTrafMonth, trafMonth, inBytes, outBytes);
    quotaManager()->setTrafMonthBytes(inBytes);
}

void StatManager::clearQuotas(bool isNewDay, bool isNewMonth)
{
    quotaManager()->clear(isNewDay && m_trafDay != 0, isNewMonth && m_trafMonth != 0);
}

void StatManager::checkQuotas(quint32 inBytes)
{
    if (m_isActivePeriod) {
        // Update quota traffic bytes
        quotaManager()->addTraf(inBytes);

        quotaManager()->checkQuotaDay(m_trafDay);
        quotaManager()->checkQuotaMonth(m_trafMonth);
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

bool StatManager::clear()
{
    sqliteDb()->beginTransaction();
    sqliteDb()->execute(StatSql::sqlClear);
    sqliteDb()->execute(StatSql::sqlVacuum);
    sqliteDb()->commitTransaction();

    clearAppIdCache();

    quotaManager()->clear();

    emit cleared();

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

bool StatManager::logProcNew(quint32 pid, const QString &appPath, qint64 unixTime)
{
    Q_ASSERT(!m_appPidPathMap.contains(pid));
    m_appPidPathMap.insert(pid, appPath);

    return getOrCreateAppId(appPath, unixTime) != INVALID_APP_ID;
}

bool StatManager::logStatTraf(quint16 procCount, const quint32 *procTrafBytes, qint64 unixTime)
{
    if (!conf() || !conf()->logStat())
        return false;

    // Active period
    updateActivePeriod();

    const bool isNewDay = updateTrafDay(unixTime);

    sqliteDb()->beginTransaction();

    // Delete old data
    if (isNewDay) {
        deleteOldTraffic(m_trafHour);
    }

    // Sum traffic bytes
    quint32 sumInBytes = 0;
    quint32 sumOutBytes = 0;

    // Insert Statements
    const QStmtList insertTrafAppStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlInsertTrafAppHour, m_trafHour)
            << getTrafficStmt(StatSql::sqlInsertTrafAppDay, m_trafDay)
            << getTrafficStmt(StatSql::sqlInsertTrafAppMonth, m_trafMonth)
            << getTrafficStmt(StatSql::sqlInsertTrafAppTotal, m_trafHour);

    const QStmtList insertTrafStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlInsertTrafHour, m_trafHour)
            << getTrafficStmt(StatSql::sqlInsertTrafDay, m_trafDay)
            << getTrafficStmt(StatSql::sqlInsertTrafMonth, m_trafMonth);

    // Update Statements
    const QStmtList updateTrafAppStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlUpdateTrafAppHour, m_trafHour)
            << getTrafficStmt(StatSql::sqlUpdateTrafAppDay, m_trafDay)
            << getTrafficStmt(StatSql::sqlUpdateTrafAppMonth, m_trafMonth)
            << getTrafficStmt(StatSql::sqlUpdateTrafAppTotal, -1);

    const QStmtList updateTrafStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlUpdateTrafHour, m_trafHour)
            << getTrafficStmt(StatSql::sqlUpdateTrafDay, m_trafDay)
            << getTrafficStmt(StatSql::sqlUpdateTrafMonth, m_trafMonth);

    for (int i = 0; i < procCount; ++i) {
        const quint32 pidFlag = *procTrafBytes++;
        const quint32 inBytes = *procTrafBytes++;
        const quint32 outBytes = *procTrafBytes++;

        logTrafBytes(insertTrafAppStmts, updateTrafAppStmts, sumInBytes, sumOutBytes, pidFlag,
                inBytes, outBytes, unixTime);
    }

    if (m_isActivePeriod) {
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

bool StatManager::logBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime)
{
    if (!conf() || !conf()->logBlockedIp())
        return false;

    bool ok;
    sqliteDb()->beginTransaction();

    const qint64 appId = getOrCreateAppId(entry.path(), unixTime, true);
    ok = (appId != INVALID_APP_ID);
    if (ok) {
        ok = createConnBlock(entry, unixTime, appId);
    }

    sqliteDb()->endTransaction(ok);

    if (ok) {
        emit connBlockAdded();
    }

    return ok;
}

bool StatManager::deleteStatApp(qint64 appId)
{
    sqliteDb()->beginTransaction();

    deleteAppStmtList({ getIdStmt(StatSql::sqlDeleteAppTrafHour, appId),
                              getIdStmt(StatSql::sqlDeleteAppTrafDay, appId),
                              getIdStmt(StatSql::sqlDeleteAppTrafMonth, appId),
                              getIdStmt(StatSql::sqlDeleteAppTrafTotal, appId) },
            getIdStmt(StatSql::sqlSelectDeletedStatAppList, appId));

    sqliteDb()->commitTransaction();

    emit appStatRemoved(appId);

    return true;
}

bool StatManager::deleteOldConnBlock()
{
    const int keepCount = ini()->blockedIpKeepCount();
    const int totalCount = m_connBlockIdMax - m_connBlockIdMin + 1;
    const int oldCount = totalCount - keepCount;
    if (oldCount <= 0)
        return false;

    deleteRangeConnBlock(m_connBlockIdMin, m_connBlockIdMin + oldCount);

    emit connRemoved();

    return true;
}

bool StatManager::deleteConn(qint64 rowIdTo, bool blocked)
{
    sqliteDb()->beginTransaction();

    if (blocked) {
        deleteRangeConnBlock(m_connBlockIdMin, rowIdTo);
    } else {
        // TODO: deleteRangeConnTraf(m_connTrafIdMin, rowIdTo);
    }

    sqliteDb()->commitTransaction();

    emit connRemoved();

    return true;
}

bool StatManager::deleteConnAll()
{
    sqliteDb()->beginTransaction();

    deleteAppStmtList({ sqliteDb()->stmt(StatSql::sqlDeleteAllConn),
                              sqliteDb()->stmt(StatSql::sqlDeleteAllConnBlock) },
            sqliteDb()->stmt(StatSql::sqlSelectDeletedAllConnAppList));

    sqliteDb()->execute(StatSql::sqlVacuum);

    sqliteDb()->commitTransaction();

    m_connBlockIdMin = m_connBlockIdMax = 0;

    emit connRemoved();

    return true;
}

bool StatManager::resetAppTrafTotals()
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlResetAppTrafTotals);
    const qint64 unixTime = DateUtil::getUnixTime();

    stmt->bindInt(1, DateUtil::getUnixHour(unixTime));

    return sqliteDb()->done(stmt);
}

bool StatManager::hasAppTraf(qint64 appId)
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlSelectStatAppExists);

    stmt->bindInt64(1, appId);
    const bool res = (stmt->step() == SqliteStmt::StepRow);
    stmt->reset();

    return res;
}

qint64 StatManager::getAppId(const QString &appPath)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlSelectAppId);

    stmt->bindText(1, appPath);
    if (stmt->step() == SqliteStmt::StepRow) {
        appId = stmt->columnInt64();
    }
    stmt->reset();

    return appId;
}

qint64 StatManager::createAppId(const QString &appPath, qint64 unixTime)
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlInsertAppId);

    stmt->bindText(1, appPath);
    stmt->bindInt64(2, unixTime);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return INVALID_APP_ID;
}

qint64 StatManager::getOrCreateAppId(const QString &appPath, qint64 unixTime, bool blocked)
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

        if (!blocked && !hasAppTraf(appId)) {
            emit appCreated(appId, appPath);
        }

        addCachedAppId(appPath, appId);
    }
    return appId;
}

bool StatManager::deleteAppId(qint64 appId)
{
    SqliteStmt *stmt = getIdStmt(StatSql::sqlDeleteAppId, appId);

    return sqliteDb()->done(stmt);
}

void StatManager::deleteOldTraffic(qint32 trafHour)
{
    QStmtList deleteTrafStmts;

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

    doStmtList(deleteTrafStmts);
}

void StatManager::getStatAppList(QStringList &list, QVector<qint64> &appIds)
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlSelectStatAppList);

    while (stmt->step() == SqliteStmt::StepRow) {
        appIds.append(stmt->columnInt64(0));
        list.append(stmt->columnText(1));
    }
    stmt->reset();
}

void StatManager::logTrafBytes(const QStmtList &insertStmtList, const QStmtList &updateStmtList,
        quint32 &sumInBytes, quint32 &sumOutBytes, quint32 pidFlag, quint32 inBytes,
        quint32 outBytes, qint64 unixTime)
{
    const bool inactive = (pidFlag & 1) != 0;
    const quint32 pid = pidFlag & ~quint32(1);

    const QString appPath = m_appPidPathMap.value(pid);

    if (Q_UNLIKELY(appPath.isEmpty())) {
        logCritical() << "UI & Driver's states mismatch! Expected processes:"
                      << m_appPidPathMap.keys() << "Got:" << pid << inactive;
        return;
    }

    if (inactive) {
        logClearApp(pid);
    }

    if (inBytes == 0 && outBytes == 0)
        return;

    const qint64 appId = getOrCreateAppId(appPath, unixTime);
    Q_ASSERT(appId != INVALID_APP_ID);

    if (m_isActivePeriod) {
        // Update or insert app bytes
        updateTrafficList(insertStmtList, updateStmtList, inBytes, outBytes, appId);
    }

    // Update sum traffic bytes
    sumInBytes += inBytes;
    sumOutBytes += outBytes;
}

void StatManager::updateTrafficList(const QStmtList &insertStmtList,
        const QStmtList &updateStmtList, quint32 inBytes, quint32 outBytes, qint64 appId)
{
    int i = 0;
    for (SqliteStmt *stmtUpdate : updateStmtList) {
        if (!updateTraffic(stmtUpdate, inBytes, outBytes, appId)) {
            SqliteStmt *stmtInsert = insertStmtList.at(i);
            if (!updateTraffic(stmtInsert, inBytes, outBytes, appId)) {
                logCritical() << "Update traffic error:" << sqliteDb()->errorMessage()
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

qint64 StatManager::insertConn(const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId)
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlInsertConn);

    stmt->bindInt64(1, appId);
    stmt->bindInt64(2, unixTime);
    stmt->bindInt(3, entry.pid());
    stmt->bindInt(4, entry.inbound());
    stmt->bindInt(5, entry.ipProto());
    stmt->bindInt(6, entry.localPort());
    stmt->bindInt(7, entry.remotePort());
    stmt->bindInt(8, entry.localIp());
    stmt->bindInt(9, entry.remoteIp());

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return 0;
}

qint64 StatManager::insertConnBlock(qint64 connId, quint8 blockReason)
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlInsertConnBlock);

    stmt->bindInt64(1, connId);
    stmt->bindInt(2, blockReason);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return 0;
}

bool StatManager::createConnBlock(const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId)
{
    const qint64 connId = insertConn(entry, unixTime, appId);
    if (connId <= 0)
        return false;

    const qint64 connBlockId = insertConnBlock(connId, entry.blockReason());
    if (connBlockId <= 0)
        return false;

    if (m_connBlockIdMax > 0) {
        m_connBlockIdMax++;
    } else {
        m_connBlockIdMin = m_connBlockIdMax = 1;
    }

    return true;
}

void StatManager::deleteRangeConnBlock(qint64 rowIdFrom, qint64 rowIdTo)
{
    deleteAppStmtList({ getId2Stmt(StatSql::sqlDeleteRangeConnForBlock, rowIdFrom, rowIdTo),
                              getId2Stmt(StatSql::sqlDeleteRangeConnBlock, rowIdFrom, rowIdTo) },
            getId2Stmt(StatSql::sqlSelectDeletedRangeConnBlockAppList, rowIdFrom, rowIdTo));

    m_connBlockIdMin = rowIdTo + 1;
    if (m_connBlockIdMin >= m_connBlockIdMax) {
        m_connBlockIdMin = m_connBlockIdMax = 0;
    }
}

void StatManager::deleteAppStmtList(const StatManager::QStmtList &stmtList, SqliteStmt *stmtAppList)
{
    // Delete Statements
    doStmtList(stmtList);

    // Delete Cached AppIds
    {
        while (stmtAppList->step() == SqliteStmt::StepRow) {
            const qint64 appId = stmtAppList->columnInt64(0);
            const QString appPath = stmtAppList->columnText(1);

            deleteAppId(appId);
            clearCachedAppId(appPath);
        }
        stmtAppList->reset();
    }
}

void StatManager::doStmtList(const QStmtList &stmtList)
{
    for (SqliteStmt *stmt : stmtList) {
        stmt->step();
        stmt->reset();
    }
}

qint32 StatManager::getTrafficTime(const char *sql, qint64 appId)
{
    qint32 trafTime = 0;

    SqliteStmt *stmt = sqliteDb()->stmt(sql);

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
    SqliteStmt *stmt = sqliteDb()->stmt(sql);

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

SqliteStmt *StatManager::getTrafficStmt(const char *sql, qint32 trafTime)
{
    SqliteStmt *stmt = sqliteDb()->stmt(sql);

    stmt->bindInt(1, trafTime);

    return stmt;
}

SqliteStmt *StatManager::getIdStmt(const char *sql, qint64 id)
{
    SqliteStmt *stmt = sqliteDb()->stmt(sql);

    stmt->bindInt64(1, id);

    return stmt;
}

SqliteStmt *StatManager::getId2Stmt(const char *sql, qint64 id1, qint64 id2)
{
    SqliteStmt *stmt = getIdStmt(sql, id1);

    stmt->bindInt64(2, id2);

    return stmt;
}
