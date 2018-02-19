#include "databasemanager.h"

#include "../conf/firewallconf.h"
#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "databasesql.h"
#include "quotamanager.h"
#include "sqlite/sqlitedb.h"
#include "sqlite/sqliteengine.h"
#include "sqlite/sqlitestmt.h"

#define INVALID_APP_ID  qint64(-1)

DatabaseManager::DatabaseManager(const QString &filePath,
                                 QuotaManager *quotaManager,
                                 QObject *parent) :
    QObject(parent),
    m_lastTrafHour(0),
    m_lastTrafDay(0),
    m_lastTrafMonth(0),
    m_filePath(filePath),
    m_quotaManager(quotaManager),
    m_conf(nullptr),
    m_sqliteDb(new SqliteDb())
{
    SqliteEngine::initialize();
}

DatabaseManager::~DatabaseManager()
{
    clearStmts();

    delete m_sqliteDb;

    SqliteEngine::shutdown();
}

void DatabaseManager::setFirewallConf(const FirewallConf *conf)
{
    m_conf = conf;

    if (m_conf && !m_conf->logStat()) {
        logClear();
    }

    initializeQuota();
}

bool DatabaseManager::initialize()
{
    const bool fileExists = FileUtil::fileExists(m_filePath);

    m_lastTrafHour = m_lastTrafDay = m_lastTrafMonth = 0;

    if (!m_sqliteDb->open(m_filePath))
        return false;

    m_sqliteDb->execute(DatabaseSql::sqlPragmas);

    return fileExists || createTables();
}

void DatabaseManager::initializeQuota()
{
    m_quotaManager->setQuotaDayBytes(
                m_conf ? qint64(m_conf->quotaDayMb()) * 1024 * 1024 : 0);
    m_quotaManager->setQuotaMonthBytes(
                m_conf ? qint64(m_conf->quotaMonthMb()) * 1024 * 1024 : 0);

    const qint64 unixTime = DateUtil::getUnixTime();
    const qint32 trafDay = DateUtil::getUnixDay(unixTime);
    const qint32 trafMonth = DateUtil::getUnixMonth(
                unixTime, m_conf ? m_conf->monthStart() : 1);

    qint64 inBytes, outBytes;

    getTraffic(DatabaseSql::sqlSelectTrafDay, trafDay, inBytes, outBytes);
    m_quotaManager->setTrafDayBytes(inBytes);

    getTraffic(DatabaseSql::sqlSelectTrafMonth, trafMonth, inBytes, outBytes);
    m_quotaManager->setTrafMonthBytes(inBytes);
}

void DatabaseManager::clear()
{
    clearAppIds();
    clearStmts();

    m_sqliteDb->close();

    FileUtil::removeFile(m_filePath);

    initialize();

    m_quotaManager->clear();
}

void DatabaseManager::clearStmts()
{
    qDeleteAll(m_sqliteStmts);
    m_sqliteStmts.clear();
}

void DatabaseManager::replaceAppIdAt(int index, qint64 appId)
{
    m_appIds.replace(index, appId);
}

void DatabaseManager::clearAppId(qint64 appId)
{
    const int index = m_appIds.indexOf(appId);

    if (index >= 0) {
        replaceAppIdAt(index, INVALID_APP_ID);
    }
}

void DatabaseManager::clearAppIds()
{
    int index = m_appIds.size();

    while (--index >= 0) {
        replaceAppIdAt(index, INVALID_APP_ID);
    }
}

void DatabaseManager::logProcNew(const QString &appPath)
{
    if (m_conf && !m_conf->logStat())
        return;

    m_sqliteDb->beginTransaction();

    qint64 appId = getAppId(appPath);
    if (appId == INVALID_APP_ID) {
        appId = createAppId(appPath);
    }

    m_sqliteDb->commitTransaction();

    m_appPaths.prepend(appPath);
    m_appIds.prepend(appId);
}

void DatabaseManager::logStatTraf(quint16 procCount, const quint8 *procBits,
                                  const quint32 *trafBytes)
{
    Q_ASSERT(procCount == m_appIds.size());

    if (m_conf && !m_conf->logStat())
        return;

    QVector<quint16> delProcIndexes;

    const qint64 unixTime = DateUtil::getUnixTime();

    const qint32 trafHour = DateUtil::getUnixHour(unixTime);
    const bool isNewHour = (trafHour != m_lastTrafHour);

    const qint32 trafDay = isNewHour ? DateUtil::getUnixDay(unixTime)
                                     : m_lastTrafDay;
    const bool isNewDay = (trafDay != m_lastTrafDay);

    const qint32 trafMonth = isNewDay
            ? DateUtil::getUnixMonth(unixTime, m_conf ? m_conf->monthStart() : 1)
            : m_lastTrafMonth;
    const bool isNewMonth = (trafMonth != m_lastTrafMonth);

    m_lastTrafHour = trafHour;
    m_lastTrafDay = trafDay;
    m_lastTrafMonth = trafMonth;

    // Initialize quotas traffic bytes
    m_quotaManager->clear(isNewDay, isNewMonth);

    m_sqliteDb->beginTransaction();

    // Insert Statemets
    const QStmtList insertTrafAppStmts = QStmtList()
            << getTrafficStmt(DatabaseSql::sqlInsertTrafAppHour, trafHour)
            << getTrafficStmt(DatabaseSql::sqlInsertTrafAppDay, trafDay)
            << getTrafficStmt(DatabaseSql::sqlInsertTrafAppMonth, trafMonth)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafAppTotal, -1);

    const QStmtList insertTrafStmts = QStmtList()
            << getTrafficStmt(DatabaseSql::sqlInsertTrafHour, trafHour)
            << getTrafficStmt(DatabaseSql::sqlInsertTrafDay, trafDay)
            << getTrafficStmt(DatabaseSql::sqlInsertTrafMonth, trafMonth);

    // Update Statemets
    const QStmtList updateTrafAppStmts = QStmtList()
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafAppHour, trafHour)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafAppDay, trafDay)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafAppMonth, trafMonth)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafAppTotal, -1);

    const QStmtList updateTrafStmts = QStmtList()
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafHour, trafHour)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafDay, trafDay)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafMonth, trafMonth);

    for (quint16 i = 0; i < procCount; ++i) {
        const bool active = procBits[i / 8] & (1 << (i & 7));
        if (!active) {
            delProcIndexes.append(i);
        }

        const quint32 *procTrafBytes = &trafBytes[i * 2];
        const quint32 inBytes = procTrafBytes[0];
        const quint32 outBytes = procTrafBytes[1];

        if (inBytes || outBytes) {
            qint64 appId = m_appIds.at(i);

            // Was the app cleared?
            if (appId == INVALID_APP_ID) {
                appId = createAppId(m_appPaths.at(i));
                replaceAppIdAt(i, appId);
            }

            // Update or insert app bytes
            updateTrafficList(insertTrafAppStmts, updateTrafAppStmts,
                              inBytes, outBytes, appId);

            // Update or insert total bytes
            updateTrafficList(insertTrafStmts, updateTrafStmts,
                              inBytes, outBytes);

            // Update quota traffic bytes
            m_quotaManager->addTraf(inBytes);
        }
    }

    // Delete old data
    if (isNewDay) {
        QStmtList deleteTrafStmts;

        // Traffic Hour
        const int trafHourKeepDays = m_conf ? m_conf->trafHourKeepDays()
                                            : DEFAULT_TRAF_HOUR_KEEP_DAYS;
        if (trafHourKeepDays >= 0) {
            const qint32 oldTrafHour = trafHour - 24 * trafHourKeepDays;

            deleteTrafStmts
                    << getTrafficStmt(DatabaseSql::sqlDeleteTrafAppHour, oldTrafHour)
                    << getTrafficStmt(DatabaseSql::sqlDeleteTrafHour, oldTrafHour);
        }

        // Traffic Day
        const int trafDayKeepDays = m_conf ? m_conf->trafDayKeepDays()
                                           : DEFAULT_TRAF_DAY_KEEP_DAYS;
        if (trafDayKeepDays >= 0) {
            const qint32 oldTrafDay = trafHour - 24 * trafDayKeepDays;

            deleteTrafStmts
                    << getTrafficStmt(DatabaseSql::sqlDeleteTrafAppDay, oldTrafDay)
                    << getTrafficStmt(DatabaseSql::sqlDeleteTrafDay, oldTrafDay);
        }

        // Traffic Month
        const int trafMonthKeepMonths = m_conf ? m_conf->trafMonthKeepMonths()
                                               : DEFAULT_TRAF_MONTH_KEEP_MONTHS;
        if (trafMonthKeepMonths >= 0) {
            const qint32 oldTrafMonth = DateUtil::addUnixMonths(
                        trafHour, -trafMonthKeepMonths);

            deleteTrafStmts
                    << getTrafficStmt(DatabaseSql::sqlDeleteTrafAppMonth, oldTrafMonth)
                    << getTrafficStmt(DatabaseSql::sqlDeleteTrafMonth, oldTrafMonth);
        }

        stepStmtList(deleteTrafStmts);
    }

    m_sqliteDb->commitTransaction();

    // Delete inactive processes
    {
        int i = delProcIndexes.size();
        while (--i >= 0) {
            const quint16 procIndex = delProcIndexes.at(i);

            m_appPaths.removeAt(procIndex);
            m_appIds.removeAt(procIndex);
        }
    }

    // Check quotas
    m_quotaManager->checkQuotaDay(trafDay);
    m_quotaManager->checkQuotaMonth(trafMonth);
}

void DatabaseManager::logClear()
{
    m_appPaths.clear();
    m_appIds.clear();
}

bool DatabaseManager::createTables()
{
    m_sqliteDb->beginTransaction();

    const bool res = m_sqliteDb->execute(DatabaseSql::sqlCreateTables);

    m_sqliteDb->commitTransaction();

    return res;
}

qint64 DatabaseManager::getAppId(const QString &appPath)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlSelectAppId);

    stmt->bindText(1, appPath);
    if (stmt->step() == SqliteStmt::StepRow) {
        appId = stmt->columnInt64();
    }
    stmt->reset();

    return appId;
}

void DatabaseManager::deleteApp(qint64 appId)
{
    clearAppId(appId);

    // Delete Statemets
    const QStmtList deleteAppStmts = QStmtList()
            << getAppStmt(DatabaseSql::sqlDeleteAppTrafHour, appId)
            << getAppStmt(DatabaseSql::sqlDeleteAppTrafDay, appId)
            << getAppStmt(DatabaseSql::sqlDeleteAppTrafMonth, appId)
            << getAppStmt(DatabaseSql::sqlDeleteAppId, appId);

    stepStmtList(deleteAppStmts);
}

void DatabaseManager::resetAppTotals()
{
    m_sqliteDb->beginTransaction();

    SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlResetAppTrafTotals);
    const qint64 unixTime = DateUtil::getUnixTime();

    stmt->bindInt(1, DateUtil::getUnixHour(unixTime));

    stmt->step();
    stmt->reset();

    m_sqliteDb->commitTransaction();
}

qint64 DatabaseManager::createAppId(const QString &appPath)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlInsertAppId);
    const qint64 unixTime = DateUtil::getUnixTime();

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

void DatabaseManager::getAppList(QStringList &list, QVector<qint64> &appIds)
{
    SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlSelectAppPaths);

    while (stmt->step() == SqliteStmt::StepRow) {
        appIds.append(stmt->columnInt64(0));
        list.append(stmt->columnText(1));
    }
    stmt->reset();
}

void DatabaseManager::updateTrafficList(const QStmtList &insertStmtList,
                                        const QStmtList &updateStmtList,
                                        quint32 inBytes, quint32 outBytes,
                                        qint64 appId)
{
    int i = 0;
    foreach (SqliteStmt *stmtUpdate, updateStmtList) {
        if (!updateTraffic(stmtUpdate, inBytes, outBytes, appId)) {
            SqliteStmt *stmtInsert = insertStmtList.at(i);
            updateTraffic(stmtInsert, inBytes, outBytes, appId);
        }
        ++i;
    }
}

bool DatabaseManager::updateTraffic(SqliteStmt *stmt, quint32 inBytes,
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

void DatabaseManager::stepStmtList(const QStmtList &stmtList)
{
    foreach (SqliteStmt *stmtDelete, stmtList) {
        stmtDelete->step();
        stmtDelete->reset();
    }
}

qint32 DatabaseManager::getTrafficTime(const char *sql, qint64 appId)
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

void DatabaseManager::getTraffic(const char *sql, qint32 trafTime,
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

SqliteStmt *DatabaseManager::getTrafficStmt(const char *sql, qint32 trafTime)
{
    SqliteStmt *stmt = getSqliteStmt(sql);

    stmt->bindInt(1, trafTime);

    return stmt;
}

SqliteStmt *DatabaseManager::getAppStmt(const char *sql, qint64 appId)
{
    SqliteStmt *stmt = getSqliteStmt(sql);

    stmt->bindInt64(1, appId);

    return stmt;
}

SqliteStmt *DatabaseManager::getSqliteStmt(const char *sql)
{
    SqliteStmt *stmt = m_sqliteStmts.value(sql);

    if (stmt == nullptr) {
        stmt = new SqliteStmt();
        stmt->prepare(m_sqliteDb->db(), sql, SqliteStmt::PreparePersistent);

        m_sqliteStmts.insert(sql, stmt);
    }

    return stmt;
}
