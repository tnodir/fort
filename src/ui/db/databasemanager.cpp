#include "databasemanager.h"

#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "databasesql.h"
#include "sqlite/sqlitedb.h"
#include "sqlite/sqliteengine.h"
#include "sqlite/sqlitestmt.h"

DatabaseManager::DatabaseManager(const QString &filePath,
                                 QObject *parent) :
    QObject(parent),
    m_lastTrafHour(0),
    m_lastTrafDay(0),
    m_lastTrafMonth(0),
    m_filePath(filePath),
    m_sqliteDb(new SqliteDb())
{
    SqliteEngine::initialize();
}

DatabaseManager::~DatabaseManager()
{
    qDeleteAll(m_sqliteStmts);

    delete m_sqliteDb;

    SqliteEngine::shutdown();
}

bool DatabaseManager::initialize()
{
    const bool fileExists = FileUtil::fileExists(m_filePath);

    if (!m_sqliteDb->open(m_filePath))
        return false;

    m_sqliteDb->execute(DatabaseSql::sqlPragmas);

    return fileExists || createTables();
}

void DatabaseManager::logProcNew(const QString &appPath, bool &isNew)
{
    qint64 appId = getAppId(appPath);
    if (appId == 0) {
        appId = createAppId(appPath);
        isNew = true;
    }

    m_appPaths.prepend(appPath);
    m_appIds.prepend(appId);
}

void DatabaseManager::logStatTraf(quint16 procCount, const quint8 *procBits,
                                  const quint32 *trafBytes)
{
    Q_ASSERT(procCount == m_appIds.size());

    QVector<quint16> delProcIndexes;

    const qint64 unixTime = DateUtil::getUnixTime();

    const qint32 trafHour = DateUtil::getUnixHour(unixTime);
    const bool isNewHour = (trafHour != m_lastTrafHour);

    const qint32 trafDay = isNewHour ? DateUtil::getUnixDay(unixTime)
                                     : m_lastTrafDay;
    const bool isNewDay = (trafDay != m_lastTrafDay);

    const qint32 trafMonth = isNewDay ? DateUtil::getUnixMonth(unixTime)
                                      : m_lastTrafMonth;

    m_lastTrafHour = trafHour;
    m_lastTrafDay = trafDay;
    m_lastTrafMonth = trafMonth;

    m_sqliteDb->beginTransaction();

    // Insert Statemets
    QStmtList insertTrafAppStmts = QStmtList()
            << getTrafficStmt(DatabaseSql::sqlInsertTrafficAppHour, trafHour)
            << getTrafficStmt(DatabaseSql::sqlInsertTrafficAppDay, trafDay)
            << getTrafficStmt(DatabaseSql::sqlInsertTrafficAppMonth, trafMonth)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafficAppTotal, -1);

    QStmtList insertTrafStmts = QStmtList()
            << getTrafficStmt(DatabaseSql::sqlInsertTrafficHour, trafHour)
            << getTrafficStmt(DatabaseSql::sqlInsertTrafficDay, trafDay)
            << getTrafficStmt(DatabaseSql::sqlInsertTrafficMonth, trafMonth);

    // Update Statemets
    QStmtList updateTrafAppStmts = QStmtList()
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafficAppHour, trafHour)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafficAppDay, trafDay)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafficAppMonth, trafMonth)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafficAppTotal, -1);

    QStmtList updateTrafStmts = QStmtList()
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafficHour, trafHour)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafficDay, trafDay)
            << getTrafficStmt(DatabaseSql::sqlUpdateTrafficMonth, trafMonth);

    for (quint16 i = 0; i < procCount; ++i) {
        const bool active = procBits[i / 8] & (1 << (i & 7));
        if (!active) {
            delProcIndexes.append(i);
        }

        const quint32 *procTrafBytes = &trafBytes[i * 2];
        const quint32 inBytes = procTrafBytes[0];
        const quint32 outBytes = procTrafBytes[1];

        if (inBytes || outBytes) {
            const qint64 appId = m_appIds.at(i);

            // Update or insert app bytes
            updateTrafficList(insertTrafAppStmts, updateTrafAppStmts,
                              inBytes, outBytes, appId);

            // Update or insert total bytes
            updateTrafficList(insertTrafStmts, updateTrafStmts,
                              inBytes, outBytes);
        }
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
    qint64 appId = 0;

    SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlSelectAppId);

    stmt->bindText(1, appPath);
    if (stmt->step() == SqliteStmt::StepRow) {
        appId = stmt->columnInt64();
    }
    stmt->reset();

    return appId;
}

qint64 DatabaseManager::createAppId(const QString &appPath)
{
    qint64 appId = 0;

    m_sqliteDb->beginTransaction();

    SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlInsertAppId);
    const qint64 unixTime = DateUtil::getUnixTime();

    stmt->bindText(1, appPath);
    stmt->bindInt64(2, unixTime);
    stmt->bindInt(3, DateUtil::getUnixHour(unixTime));

    if (stmt->step() == SqliteStmt::StepDone) {
        appId = m_sqliteDb->lastInsertRowid();
    }
    stmt->reset();

    m_sqliteDb->commitTransaction();

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
