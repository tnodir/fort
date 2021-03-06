#include "statmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../conf/firewallconf.h"
#include "../fortcommon.h"
#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "../util/osutil.h"
#include "quotamanager.h"
#include "statsql.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_STAT_MANAGER)
Q_LOGGING_CATEGORY(CLOG_STAT_MANAGER, "stat")

#define logWarning()  qCWarning(CLOG_STAT_MANAGER, )
#define logCritical() qCCritical(CLOG_STAT_MANAGER, )

#define DATABASE_USER_VERSION 3

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

StatManager::StatManager(const QString &filePath, QuotaManager *quotaManager, QObject *parent) :
    QObject(parent),
    m_isActivePeriodSet(false),
    m_isActivePeriod(false),
    m_quotaManager(quotaManager),
    m_sqliteDb(new SqliteDb(filePath))
{
}

StatManager::~StatManager()
{
    clearStmts();

    delete m_sqliteDb;
}

void StatManager::setFirewallConf(const FirewallConf *conf)
{
    m_conf = conf;

    if (!m_conf || !m_conf->logStat()) {
        logClear();
    }

    initializeActivePeriod();
    initializeQuota();
}

bool StatManager::initialize()
{
    m_lastTrafHour = m_lastTrafDay = m_lastTrafMonth = 0;

    if (!m_sqliteDb->open()) {
        logCritical() << "File open error:" << m_sqliteDb->filePath() << m_sqliteDb->errorMessage();
        return false;
    }

    if (!m_sqliteDb->migrate(
                ":/stat/migrations", nullptr, DATABASE_USER_VERSION, true, true, &migrateFunc)) {
        logCritical() << "Migration error" << m_sqliteDb->filePath();
        return false;
    }

    return true;
}

void StatManager::initializeActivePeriod()
{
    m_isActivePeriodSet = false;

    if (!m_conf)
        return;

    DateUtil::parseTime(m_conf->activePeriodFrom(), activePeriodFromHour, activePeriodFromMinute);
    DateUtil::parseTime(m_conf->activePeriodTo(), activePeriodToHour, activePeriodToMinute);
}

void StatManager::initializeQuota()
{
    if (!m_conf)
        return;

    m_quotaManager->setQuotaDayBytes(qint64(m_conf->quotaDayMb()) * 1024 * 1024);
    m_quotaManager->setQuotaMonthBytes(qint64(m_conf->quotaMonthMb()) * 1024 * 1024);

    const qint64 unixTime = DateUtil::getUnixTime();
    const qint32 trafDay = DateUtil::getUnixDay(unixTime);
    const qint32 trafMonth = DateUtil::getUnixMonth(unixTime, m_conf->monthStart());

    qint64 inBytes, outBytes;

    getTraffic(StatSql::sqlSelectTrafDay, trafDay, inBytes, outBytes);
    m_quotaManager->setTrafDayBytes(inBytes);

    getTraffic(StatSql::sqlSelectTrafMonth, trafMonth, inBytes, outBytes);
    m_quotaManager->setTrafMonthBytes(inBytes);
}

void StatManager::clear()
{
    clearAppIdCache();
    clearStmts();

    m_sqliteDb->close();

    FileUtil::removeFile(m_sqliteDb->filePath());

    initialize();

    m_quotaManager->clear();
}

void StatManager::clearStmts()
{
    qDeleteAll(m_sqliteStmts);
    m_sqliteStmts.clear();
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
    if (!m_conf || !m_conf->logStat())
        return false;

    const qint32 trafHour = DateUtil::getUnixHour(unixTime);
    const bool isNewHour = (trafHour != m_lastTrafHour);

    const qint32 trafDay = isNewHour ? DateUtil::getUnixDay(unixTime) : m_lastTrafDay;
    const bool isNewDay = (trafDay != m_lastTrafDay);

    const qint32 trafMonth =
            isNewDay ? DateUtil::getUnixMonth(unixTime, m_conf->monthStart()) : m_lastTrafMonth;
    const bool isNewMonth = (trafMonth != m_lastTrafMonth);

    // Initialize quotas traffic bytes
    m_quotaManager->clear(isNewDay && m_lastTrafDay, isNewMonth && m_lastTrafMonth);

    m_lastTrafHour = trafHour;
    m_lastTrafDay = trafDay;
    m_lastTrafMonth = trafMonth;

    m_sqliteDb->beginTransaction();

    // Sum traffic bytes
    quint32 sumInBytes = 0;
    quint32 sumOutBytes = 0;

    // Active period
    const qint32 currentTick = OsUtil::getTickCount();
    if (!m_isActivePeriodSet || qAbs(currentTick - m_lastTick) >= ACTIVE_PERIOD_CHECK_SECS) {
        m_lastTick = currentTick;

        m_isActivePeriodSet = true;
        m_isActivePeriod = true;

        if (m_conf->activePeriodEnabled()) {
            const QTime now = QTime::currentTime();

            m_isActivePeriod = FortCommon::isTimeInPeriod(quint8(now.hour()), quint8(now.minute()),
                    activePeriodFromHour, activePeriodFromMinute, activePeriodToHour,
                    activePeriodToMinute);
        }
    }

    // Insert Statements
    const QStmtList insertTrafAppStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlInsertTrafAppHour, trafHour)
            << getTrafficStmt(StatSql::sqlInsertTrafAppDay, trafDay)
            << getTrafficStmt(StatSql::sqlInsertTrafAppMonth, trafMonth)
            << getTrafficStmt(StatSql::sqlInsertTrafAppTotal, trafHour);

    const QStmtList insertTrafStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlInsertTrafHour, trafHour)
            << getTrafficStmt(StatSql::sqlInsertTrafDay, trafDay)
            << getTrafficStmt(StatSql::sqlInsertTrafMonth, trafMonth);

    // Update Statements
    const QStmtList updateTrafAppStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlUpdateTrafAppHour, trafHour)
            << getTrafficStmt(StatSql::sqlUpdateTrafAppDay, trafDay)
            << getTrafficStmt(StatSql::sqlUpdateTrafAppMonth, trafMonth)
            << getTrafficStmt(StatSql::sqlUpdateTrafAppTotal, -1);

    const QStmtList updateTrafStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlUpdateTrafHour, trafHour)
            << getTrafficStmt(StatSql::sqlUpdateTrafDay, trafDay)
            << getTrafficStmt(StatSql::sqlUpdateTrafMonth, trafMonth);

    for (int i = 0; i < procCount; ++i) {
        const quint32 pidFlag = *procTrafBytes++;
        const bool inactive = (pidFlag & 1) != 0;
        const quint32 pid = pidFlag & ~quint32(1);

        const quint32 inBytes = *procTrafBytes++;
        const quint32 outBytes = *procTrafBytes++;

        const QString appPath = m_appPidPathMap.value(pid);

        if (inactive) {
            logClearApp(pid);
        }

        if (Q_UNLIKELY(appPath.isEmpty())) {
            logCritical() << "UI & Driver's states mismatch! Expected processes:"
                          << m_appPidPathMap.keys() << "Got:" << procCount << "(" << i << pid
                          << inactive << ")";
            continue;
        }

        if (inBytes != 0 || outBytes != 0) {
            const qint64 appId = getOrCreateAppId(appPath, unixTime);
            Q_ASSERT(appId != INVALID_APP_ID);

            if (m_isActivePeriod) {
                // Update or insert app bytes
                updateTrafficList(insertTrafAppStmts, updateTrafAppStmts, inBytes, outBytes, appId);
            }

            // Update sum traffic bytes
            sumInBytes += inBytes;
            sumOutBytes += outBytes;
        }
    }

    if (m_isActivePeriod) {
        // Update or insert total bytes
        updateTrafficList(insertTrafStmts, updateTrafStmts, sumInBytes, sumOutBytes);

        // Update quota traffic bytes
        m_quotaManager->addTraf(sumInBytes);
    }

    // Delete old data
    if (isNewDay) {
        QStmtList deleteTrafStmts;

        // Traffic Hour
        const int trafHourKeepDays = m_conf->trafHourKeepDays();
        if (trafHourKeepDays >= 0) {
            const qint32 oldTrafHour = trafHour - 24 * trafHourKeepDays;

            deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppHour, oldTrafHour)
                            << getTrafficStmt(StatSql::sqlDeleteTrafHour, oldTrafHour);
        }

        // Traffic Day
        const int trafDayKeepDays = m_conf->trafDayKeepDays();
        if (trafDayKeepDays >= 0) {
            const qint32 oldTrafDay = trafHour - 24 * trafDayKeepDays;

            deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppDay, oldTrafDay)
                            << getTrafficStmt(StatSql::sqlDeleteTrafDay, oldTrafDay);
        }

        // Traffic Month
        const int trafMonthKeepMonths = m_conf->trafMonthKeepMonths();
        if (trafMonthKeepMonths >= 0) {
            const qint32 oldTrafMonth = DateUtil::addUnixMonths(trafHour, -trafMonthKeepMonths);

            deleteTrafStmts << getTrafficStmt(StatSql::sqlDeleteTrafAppMonth, oldTrafMonth)
                            << getTrafficStmt(StatSql::sqlDeleteTrafMonth, oldTrafMonth);
        }

        stepStmtList(deleteTrafStmts);
    }

    m_sqliteDb->commitTransaction();

    // Check quotas
    if (m_isActivePeriod) {
        m_quotaManager->checkQuotaDay(trafDay);
        m_quotaManager->checkQuotaMonth(trafMonth);
    }

    // Notify about sum traffic bytes
    emit trafficAdded(unixTime, sumInBytes, sumOutBytes);

    return true;
}

bool StatManager::logBlockedIp(bool inbound, quint8 blockReason, quint8 ipProto, quint16 localPort,
        quint16 remotePort, quint32 localIp, quint32 remoteIp, quint32 pid, const QString &appPath,
        qint64 unixTime)
{
    if (!m_conf || !m_conf->logBlockedIp())
        return false;

    bool ok;
    m_sqliteDb->beginTransaction();

    const qint64 appId = getOrCreateAppId(appPath, unixTime, true);
    ok = (appId != INVALID_APP_ID);
    if (ok) {
        const qint64 connId = createConn(
                inbound, ipProto, localPort, remotePort, localIp, remoteIp, pid, unixTime, appId);
        ok = (connId > 0);
        if (ok) {
            ok = createConnBlock(connId, blockReason);
        }
    }

    m_sqliteDb->endTransaction(ok);
    return ok;
}

void StatManager::deleteStatApp(qint64 appId)
{
    m_sqliteDb->beginTransaction();

    deleteAppStmtList(getIdStmt(StatSql::sqlSelectDeletedStatAppPaths, appId),
            { getIdStmt(StatSql::sqlDeleteStatAppId, appId),
                    getIdStmt(StatSql::sqlDeleteAppTrafHour, appId),
                    getIdStmt(StatSql::sqlDeleteAppTrafDay, appId),
                    getIdStmt(StatSql::sqlDeleteAppTrafMonth, appId),
                    getIdStmt(StatSql::sqlDeleteAppTrafTotal, appId) });

    m_sqliteDb->commitTransaction();
}

bool StatManager::deleteOldConnBlock()
{
    const int keepCount = m_conf->blockedIpKeepCount();
    const qint64 connId =
            m_sqliteDb->executeEx(StatSql::sqlSelectOldConnBlock, { keepCount }).toLongLong();
    if (connId > 0) {
        deleteRangeConnBlock(1, connId);
        return true;
    }
    return false;
}

bool StatManager::deleteConn(qint64 connId, bool blocked)
{
    m_sqliteDb->beginTransaction();

    if (blocked) {
        deleteRangeConnBlock(connId, connId);
    } else {
        // TODO: deleteRangeConnTraf(connId, connId);
    }

    m_sqliteDb->commitTransaction();

    return true;
}

void StatManager::deleteConns()
{
    m_sqliteDb->beginTransaction();

    deleteAppStmtList(getSqliteStmt(StatSql::sqlSelectDeletedAllConnAppPaths),
            { getSqliteStmt(StatSql::sqlDeleteAllConnAppId),
                    getSqliteStmt(StatSql::sqlDeleteAllConn),
                    getSqliteStmt(StatSql::sqlDeleteAllConnBlock) });

    m_sqliteDb->commitTransaction();
}

void StatManager::resetAppTrafTotals()
{
    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlResetAppTrafTotals);
    const qint64 unixTime = DateUtil::getUnixTime();

    stmt->bindInt(1, DateUtil::getUnixHour(unixTime));

    stmt->step();
    stmt->reset();
}

qint64 StatManager::getAppId(const QString &appPath)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlSelectAppId);

    stmt->bindText(1, appPath);
    if (stmt->step() == SqliteStmt::StepRow) {
        appId = stmt->columnInt64();
    }
    stmt->reset();

    return appId;
}

qint64 StatManager::createAppId(const QString &appPath, qint64 unixTime, bool blocked)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlInsertAppId);

    stmt->bindText(1, appPath);
    stmt->bindInt64(2, unixTime);

    if (stmt->step() == SqliteStmt::StepDone) {
        appId = m_sqliteDb->lastInsertRowid();

        if (!blocked) {
            emit appCreated(appId, appPath);
        }
    }
    stmt->reset();

    return appId;
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
            appId = createAppId(appPath, unixTime, blocked);
        }

        addCachedAppId(appPath, appId);
    }
    return appId;
}

void StatManager::getStatAppList(QStringList &list, QVector<qint64> &appIds)
{
    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlSelectStatAppPaths);

    while (stmt->step() == SqliteStmt::StepRow) {
        appIds.append(stmt->columnInt64(0));
        list.append(stmt->columnText(1));
    }
    stmt->reset();
}

void StatManager::updateTrafficList(const QStmtList &insertStmtList,
        const QStmtList &updateStmtList, quint32 inBytes, quint32 outBytes, qint64 appId)
{
    int i = 0;
    for (SqliteStmt *stmtUpdate : updateStmtList) {
        if (!updateTraffic(stmtUpdate, inBytes, outBytes, appId)) {
            SqliteStmt *stmtInsert = insertStmtList.at(i);
            if (!updateTraffic(stmtInsert, inBytes, outBytes, appId)) {
                logCritical() << "Update traffic error:" << m_sqliteDb->errorMessage()
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

    const SqliteStmt::StepResult res = stmt->step();
    const bool ok = res == SqliteStmt::StepDone && m_sqliteDb->changes() != 0;
    stmt->reset();

    return ok;
}

qint64 StatManager::createConn(bool inbound, quint8 ipProto, quint16 localPort, quint16 remotePort,
        quint32 localIp, quint32 remoteIp, quint32 pid, qint64 unixTime, qint64 appId)
{
    qint64 connId = 0;

    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlInsertConn);

    stmt->bindInt64(1, appId);
    stmt->bindInt64(2, unixTime);
    stmt->bindInt(3, pid);
    stmt->bindInt(4, inbound);
    stmt->bindInt(5, ipProto);
    stmt->bindInt(6, localPort);
    stmt->bindInt(7, remotePort);
    stmt->bindInt(8, localIp);
    stmt->bindInt(9, remoteIp);

    if (stmt->step() == SqliteStmt::StepDone) {
        connId = m_sqliteDb->lastInsertRowid();
    }
    stmt->reset();

    return connId;
}

bool StatManager::createConnBlock(qint64 connId, quint8 blockReason)
{
    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlInsertConnBlock);

    stmt->bindInt64(1, connId);
    stmt->bindInt(2, blockReason);

    const bool ok = (stmt->step() == SqliteStmt::StepDone);
    stmt->reset();

    return ok;
}

void StatManager::deleteRangeConnBlock(qint64 connIdFrom, qint64 connIdTo)
{
    deleteAppStmtList(getId2Stmt(StatSql::sqlSelectDeletedRangeConnAppPaths, connIdFrom, connIdTo),
            { getId2Stmt(StatSql::sqlDeleteRangeConnAppId, connIdFrom, connIdTo),
                    getId2Stmt(StatSql::sqlDeleteRangeConn, connIdFrom, connIdTo),
                    getId2Stmt(StatSql::sqlDeleteRangeConnBlock, connIdFrom, connIdTo) });
}

void StatManager::deleteAppStmtList(
        SqliteStmt *stmtAppPaths, const StatManager::QStmtList &stmtList)
{
    // Delete Cached AppIds
    {
        while (stmtAppPaths->step() == SqliteStmt::StepRow) {
            const QString appPath = stmtAppPaths->columnText(0);
            clearCachedAppId(appPath);
        }
        stmtAppPaths->reset();
    }

    // Delete Statements
    stepStmtList(stmtList);
}

void StatManager::stepStmtList(const QStmtList &stmtList)
{
    for (SqliteStmt *stmt : stmtList) {
        stmt->step();
        stmt->reset();
    }
}

qint32 StatManager::getTrafficTime(const char *sql, qint64 appId)
{
    qint32 trafTime = 0;

    SqliteStmt *stmt = getSqliteStmt(sql);

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
    SqliteStmt *stmt = getSqliteStmt(sql);

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
    SqliteStmt *stmt = getSqliteStmt(sql);

    stmt->bindInt(1, trafTime);

    return stmt;
}

SqliteStmt *StatManager::getIdStmt(const char *sql, qint64 id)
{
    SqliteStmt *stmt = getSqliteStmt(sql);

    stmt->bindInt64(1, id);

    return stmt;
}

SqliteStmt *StatManager::getId2Stmt(const char *sql, qint64 id1, qint64 id2)
{
    SqliteStmt *stmt = getIdStmt(sql, id1);

    stmt->bindInt64(2, id2);

    return stmt;
}

SqliteStmt *StatManager::getSqliteStmt(const char *sql)
{
    SqliteStmt *stmt = m_sqliteStmts.value(sql);

    if (stmt == nullptr) {
        stmt = new SqliteStmt();
        stmt->prepare(m_sqliteDb->db(), sql, SqliteStmt::PreparePersistent);

        m_sqliteStmts.insert(sql, stmt);
    }

    return stmt;
}
