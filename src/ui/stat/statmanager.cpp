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
Q_LOGGING_CATEGORY(CLOG_STAT_MANAGER, "fort.statManager")

#define logWarning() qCWarning(CLOG_STAT_MANAGER,)
#define logCritical() qCCritical(CLOG_STAT_MANAGER,)

#define DATABASE_USER_VERSION   2

#define ACTIVE_PERIOD_CHECK_SECS (60 * OS_TICKS_PER_SECOND)

namespace {

bool migrateFunc(SqliteDb *db, int version, bool isNewDb, void *ctx)
{
    Q_UNUSED(db);
    Q_UNUSED(version);
    Q_UNUSED(isNewDb);
    Q_UNUSED(ctx);

#if 0
    if (version == 2) {
        // Fix statistics dates to use UTC
        const qint64 unixTime = DateUtil::getUnixTime();
        const QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();

        const qint32 unixDay = DateUtil::getUnixDay(unixTime);
        const qint32 localDay = DateUtil::getUnixHour(
                    QDateTime(date).toSecsSinceEpoch());

        if (unixDay != localDay) {
            const QVariantList vars = QVariantList() << (unixDay - localDay);
            db->executeEx("UPDATE traffic_app_day SET traf_time = traf_time + ?1;", vars);
            db->executeEx("UPDATE traffic_day SET traf_time = traf_time + ?1;", vars);
            db->executeEx("UPDATE traffic_app_month SET traf_time = traf_time + ?1;", vars);
            db->executeEx("UPDATE traffic_month SET traf_time = traf_time + ?1;", vars);
        }
    }
#endif

    return true;
}

}

StatManager::StatManager(const QString &filePath,
                         QuotaManager *quotaManager,
                         QObject *parent) :
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
        logCritical() << "File open error:" << m_sqliteDb->filePath()
                      << m_sqliteDb->errorMessage();
        return false;
    }

    m_sqliteDb->execute(StatSql::sqlPragmas);

    if (!m_sqliteDb->migrate(":/stat/migrations", DATABASE_USER_VERSION,
                             true, true, &migrateFunc)) {
        logCritical() << "Migration error" << m_sqliteDb->filePath();
        return false;
    }

    return true;
}

void StatManager::initializeActivePeriod()
{
    m_isActivePeriodSet = false;

    if (!m_conf) return;

    DateUtil::parseTime(m_conf->activePeriodFrom(),
                        activePeriodFromHour, activePeriodFromMinute);
    DateUtil::parseTime(m_conf->activePeriodTo(),
                        activePeriodToHour, activePeriodToMinute);
}

void StatManager::initializeQuota()
{
    if (!m_conf) return;

    m_quotaManager->setQuotaDayBytes(qint64(m_conf->quotaDayMb()) * 1024 * 1024);
    m_quotaManager->setQuotaMonthBytes(qint64(m_conf->quotaMonthMb()) * 1024 * 1024);

    const qint64 unixTime = DateUtil::getUnixTime();
    const qint32 trafDay = DateUtil::getUnixDay(unixTime);
    const qint32 trafMonth = DateUtil::getUnixMonth(
                unixTime, m_conf->monthStart());

    qint64 inBytes, outBytes;

    getTraffic(StatSql::sqlSelectTrafDay, trafDay, inBytes, outBytes);
    m_quotaManager->setTrafDayBytes(inBytes);

    getTraffic(StatSql::sqlSelectTrafMonth, trafMonth, inBytes, outBytes);
    m_quotaManager->setTrafMonthBytes(inBytes);
}

void StatManager::clear()
{
    clearAppIds();
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

void StatManager::replaceAppPathAt(int index, const QString &appPath)
{
    m_appPaths.replace(index, appPath);
}

void StatManager::replaceAppIdAt(int index, qint64 appId)
{
    m_appIds.replace(index, appId);
}

void StatManager::clearAppId(qint64 appId)
{
    const int index = m_appIds.indexOf(appId);

    if (index >= 0) {
        replaceAppIdAt(index, INVALID_APP_ID);
    }
}

void StatManager::clearAppIds()
{
    int index = m_appIds.size();

    while (--index >= 0) {
        replaceAppIdAt(index, INVALID_APP_ID);
    }
}

void StatManager::logClear()
{
    m_appFreeIndex = INVALID_APP_INDEX;
    m_appFreeIndexes.clear();
    m_appIndexes.clear();
    m_appPaths.clear();
    m_appIds.clear();
}

void StatManager::logClearApp(quint32 pid, int index)
{
    m_appIndexes.remove(pid);

    if (index == m_appFreeIndexes.size() - 1) {
        // Chop last index
        m_appFreeIndexes.removeLast();
        m_appPaths.removeLast();
        m_appIds.removeLast();
    } else {
        // Reuse index later
        m_appFreeIndexes[index] = m_appFreeIndex;
        m_appFreeIndex = qint16(index);

        replaceAppPathAt(index, QString());
        replaceAppIdAt(index, INVALID_APP_ID);
    }
}

void StatManager::logProcNew(quint32 pid, const QString &appPath)
{
    Q_ASSERT(!m_appIndexes.contains(pid));

    // Get appId
    m_sqliteDb->beginTransaction();

    qint64 appId = getAppId(appPath);
    if (appId == INVALID_APP_ID) {
        const qint64 unixTime = DateUtil::getUnixTime();
        appId = createAppId(appPath, unixTime);
    }

    m_sqliteDb->commitTransaction();

    // Add process
    qint16 procIndex = m_appFreeIndex;
    if (procIndex != INVALID_APP_INDEX) {
        m_appFreeIndex = m_appFreeIndexes[procIndex];
        m_appFreeIndexes[procIndex] = INVALID_APP_INDEX;

        replaceAppPathAt(procIndex, appPath);
        replaceAppIdAt(procIndex, appId);
    } else {
        procIndex = qint16(m_appFreeIndexes.size());
        m_appFreeIndexes.append(INVALID_APP_INDEX);

        m_appPaths.append(appPath);
        m_appIds.append(appId);
    }

    m_appIndexes.insert(pid, procIndex);
}

void StatManager::logStatTraf(quint16 procCount, qint64 unixTime,
                              const quint32 *procTrafBytes)
{
    if (!m_conf || !m_conf->logStat())
        return;

    const qint32 trafHour = DateUtil::getUnixHour(unixTime);
    const bool isNewHour = (trafHour != m_lastTrafHour);

    const qint32 trafDay = isNewHour ? DateUtil::getUnixDay(unixTime)
                                     : m_lastTrafDay;
    const bool isNewDay = (trafDay != m_lastTrafDay);

    const qint32 trafMonth = isNewDay
            ? DateUtil::getUnixMonth(unixTime, m_conf->monthStart())
            : m_lastTrafMonth;
    const bool isNewMonth = (trafMonth != m_lastTrafMonth);

    // Initialize quotas traffic bytes
    m_quotaManager->clear(isNewDay && m_lastTrafDay,
                          isNewMonth && m_lastTrafMonth);

    m_lastTrafHour = trafHour;
    m_lastTrafDay = trafDay;
    m_lastTrafMonth = trafMonth;

    m_sqliteDb->beginTransaction();

    // Sum traffic bytes
    quint32 sumInBytes = 0;
    quint32 sumOutBytes = 0;

    // Active period
    const qint32 currentTick = OsUtil::getTickCount();
    if (!m_isActivePeriodSet
            || qAbs(currentTick - m_lastTick) >= ACTIVE_PERIOD_CHECK_SECS) {
        m_lastTick = currentTick;

        m_isActivePeriodSet = true;
        m_isActivePeriod = true;

        if (m_conf->activePeriodEnabled()) {
            const QTime now = QTime::currentTime();

            m_isActivePeriod = FortCommon::isTimeInPeriod(
                        quint8(now.hour()), quint8(now.minute()),
                        activePeriodFromHour, activePeriodFromMinute,
                        activePeriodToHour, activePeriodToMinute);
        }
    }

    // Insert Statements
    const QStmtList insertTrafAppStmts = QStmtList()
            << getTrafficStmt(StatSql::sqlInsertTrafAppHour, trafHour)
            << getTrafficStmt(StatSql::sqlInsertTrafAppDay, trafDay)
            << getTrafficStmt(StatSql::sqlInsertTrafAppMonth, trafMonth)
            << getTrafficStmt(StatSql::sqlUpdateTrafAppTotal, -1);

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
        quint32 pid = *procTrafBytes++;
        const bool inactive = (pid & 1) != 0;
        const quint32 inBytes = *procTrafBytes++;
        const quint32 outBytes = *procTrafBytes++;

        if (inactive) {
            pid ^= 1;
        }

        const int procIndex = m_appIndexes.value(pid, INVALID_APP_INDEX);
        if (Q_UNLIKELY(procIndex == INVALID_APP_INDEX)) {
            logCritical() << "UI & Driver's states mismatch! Expected processes:"
                          << m_appIndexes.keys() << "Got:" << procCount
                          << "(" << i << pid << inactive << ")";
            abort();
        }

        if (inBytes || outBytes) {
            qint64 appId = m_appIds.at(procIndex);

            // Was the app cleared?
            if (appId == INVALID_APP_ID) {
                appId = createAppId(m_appPaths.at(procIndex), unixTime);
                replaceAppIdAt(procIndex, appId);
            }

            if (m_isActivePeriod) {
                // Update or insert app bytes
                updateTrafficList(insertTrafAppStmts, updateTrafAppStmts,
                                  inBytes, outBytes, appId);
            }

            // Update sum traffic bytes
            sumInBytes += inBytes;
            sumOutBytes += outBytes;
        }

        if (inactive) {
            logClearApp(pid, procIndex);
        }
    }

    if (m_isActivePeriod) {
        // Update or insert total bytes
        updateTrafficList(insertTrafStmts, updateTrafStmts,
                          sumInBytes, sumOutBytes);

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

            deleteTrafStmts
                    << getTrafficStmt(StatSql::sqlDeleteTrafAppHour, oldTrafHour)
                    << getTrafficStmt(StatSql::sqlDeleteTrafHour, oldTrafHour);
        }

        // Traffic Day
        const int trafDayKeepDays = m_conf->trafDayKeepDays();
        if (trafDayKeepDays >= 0) {
            const qint32 oldTrafDay = trafHour - 24 * trafDayKeepDays;

            deleteTrafStmts
                    << getTrafficStmt(StatSql::sqlDeleteTrafAppDay, oldTrafDay)
                    << getTrafficStmt(StatSql::sqlDeleteTrafDay, oldTrafDay);
        }

        // Traffic Month
        const int trafMonthKeepMonths = m_conf->trafMonthKeepMonths();
        if (trafMonthKeepMonths >= 0) {
            const qint32 oldTrafMonth = DateUtil::addUnixMonths(
                        trafHour, -trafMonthKeepMonths);

            deleteTrafStmts
                    << getTrafficStmt(StatSql::sqlDeleteTrafAppMonth, oldTrafMonth)
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
}

void StatManager::deleteApp(qint64 appId)
{
    clearAppId(appId);

    // Delete Statements
    const QStmtList deleteAppStmts = QStmtList()
            << getAppStmt(StatSql::sqlDeleteAppTrafHour, appId)
            << getAppStmt(StatSql::sqlDeleteAppTrafDay, appId)
            << getAppStmt(StatSql::sqlDeleteAppTrafMonth, appId)
            << getAppStmt(StatSql::sqlDeleteAppId, appId);

    stepStmtList(deleteAppStmts);
}

void StatManager::resetAppTotals()
{
    m_sqliteDb->beginTransaction();

    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlResetAppTrafTotals);
    const qint64 unixTime = DateUtil::getUnixTime();

    stmt->bindInt(1, DateUtil::getUnixHour(unixTime));

    stmt->step();
    stmt->reset();

    m_sqliteDb->commitTransaction();
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

qint64 StatManager::createAppId(const QString &appPath, qint64 unixTime)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlInsertAppId);

    stmt->bindText(1, appPath);
    stmt->bindInt64(2, unixTime);
    stmt->bindInt(3, DateUtil::getUnixHour(unixTime));

    if (stmt->step() == SqliteStmt::StepDone) {
        appId = m_sqliteDb->lastInsertRowid();

        emit appCreated(appId, appPath);
    }
    stmt->reset();

    return appId;
}

void StatManager::getAppList(QStringList &list, QVector<qint64> &appIds)
{
    SqliteStmt *stmt = getSqliteStmt(StatSql::sqlSelectAppPaths);

    while (stmt->step() == SqliteStmt::StepRow) {
        appIds.append(stmt->columnInt64(0));
        list.append(stmt->columnText(1));
    }
    stmt->reset();
}

void StatManager::updateTrafficList(const QStmtList &insertStmtList,
                                    const QStmtList &updateStmtList,
                                    quint32 inBytes, quint32 outBytes,
                                    qint64 appId)
{
    int i = 0;
    for (SqliteStmt *stmtUpdate : updateStmtList) {
        if (!updateTraffic(stmtUpdate, inBytes, outBytes, appId)) {
            SqliteStmt *stmtInsert = insertStmtList.at(i);
            if (!updateTraffic(stmtInsert, inBytes, outBytes, appId)) {
                logCritical() << "Update traffic error:"
                              << m_sqliteDb->errorMessage();
            }
        }
        ++i;
    }
}

bool StatManager::updateTraffic(SqliteStmt *stmt, quint32 inBytes,
                                quint32 outBytes, qint64 appId)
{
    stmt->bindInt64(2, inBytes);
    stmt->bindInt64(3, outBytes);

    if (appId != 0) {
        stmt->bindInt64(4, appId);
    }

    const SqliteStmt::StepResult res = stmt->step();

    stmt->reset();

    return res == SqliteStmt::StepDone
            && m_sqliteDb->changes() != 0;
}

void StatManager::stepStmtList(const QStmtList &stmtList)
{
    for (SqliteStmt *stmtDelete : stmtList) {
        stmtDelete->step();
        stmtDelete->reset();
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

void StatManager::getTraffic(const char *sql, qint32 trafTime,
                             qint64 &inBytes, qint64 &outBytes,
                             qint64 appId)
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

SqliteStmt *StatManager::getAppStmt(const char *sql, qint64 appId)
{
    SqliteStmt *stmt = getSqliteStmt(sql);

    stmt->bindInt64(1, appId);

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
