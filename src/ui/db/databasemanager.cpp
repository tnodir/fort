#include "databasemanager.h"

#include <QDateTime>

#include "../util/fileutil.h"
#include "databasesql.h"
#include "sqlite/sqliteengine.h"
#include "sqlite/sqlitedb.h"
#include "sqlite/sqlitestmt.h"

DatabaseManager::DatabaseManager(const QString &filePath,
                                 QObject *parent) :
    QObject(parent),
    m_lastUnixHour(0),
    m_lastUnixDay(0),
    m_lastUnixMonth(0),
    m_filePath(filePath),
    m_sqliteDb(new SqliteDb())
{
    SqliteEngine::initialize();
}

DatabaseManager::~DatabaseManager()
{
    qDeleteAll(m_sqliteStmts.values());

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

void DatabaseManager::addApp(const QString &appPath, bool &isNew)
{
    const qint64 appId = getAppId(appPath, isNew);

    m_appIds.append(appId);
}

void DatabaseManager::addTraffic(quint16 procCount, const quint8 *procBits,
                                 const quint32 *trafBytes)
{
    QVector<quint16> delProcIndexes;

    const qint64 unixTime = QDateTime::currentSecsSinceEpoch();

    const qint32 unixHour = qint32(unixTime / 3600);
    const bool isNewHour = (unixHour != m_lastUnixHour);

    const qint32 unixDay = isNewHour ? getUnixDay(unixTime)
                                     : m_lastUnixDay;
    const bool isNewDay = (unixDay != m_lastUnixDay);

    const qint32 unixMonth = isNewDay ? getUnixMonth(unixTime)
                                      : m_lastUnixMonth;
    const bool isNewMonth = (unixMonth != m_lastUnixMonth);

    SqliteStmt *stmtInsertAppHour = nullptr;
    SqliteStmt *stmtInsertAppDay = nullptr;
    SqliteStmt *stmtInsertAppMonth = nullptr;

    SqliteStmt *stmtInsertHour = nullptr;
    SqliteStmt *stmtInsertDay = nullptr;
    SqliteStmt *stmtInsertMonth = nullptr;

    if (isNewHour) {
        m_lastUnixHour = unixHour;

        stmtInsertAppHour = getSqliteStmt(DatabaseSql::sqlInsertTrafficAppHour);
        stmtInsertHour = getSqliteStmt(DatabaseSql::sqlInsertTrafficHour);

        stmtInsertAppHour->bindInt(1, unixHour);
        stmtInsertHour->bindInt(1, unixHour);

        if (isNewDay) {
            m_lastUnixDay = unixDay;

            stmtInsertAppDay = getSqliteStmt(DatabaseSql::sqlInsertTrafficAppDay);
            stmtInsertDay = getSqliteStmt(DatabaseSql::sqlInsertTrafficDay);

            stmtInsertAppDay->bindInt(1, unixDay);
            stmtInsertDay->bindInt(1, unixDay);

            if (isNewMonth) {
                m_lastUnixMonth = unixMonth;

                stmtInsertAppMonth = getSqliteStmt(DatabaseSql::sqlInsertTrafficAppMonth);
                stmtInsertMonth = getSqliteStmt(DatabaseSql::sqlInsertTrafficMonth);

                stmtInsertAppMonth->bindInt(1, unixMonth);
                stmtInsertMonth->bindInt(1, unixMonth);
            }
        }
    }

    SqliteStmt *stmtUpdateAppHour = getSqliteStmt(DatabaseSql::sqlUpdateTrafficAppHour);
    SqliteStmt *stmtUpdateAppDay = getSqliteStmt(DatabaseSql::sqlUpdateTrafficAppDay);
    SqliteStmt *stmtUpdateAppMonth = getSqliteStmt(DatabaseSql::sqlUpdateTrafficAppMonth);

    SqliteStmt *stmtUpdateHour = getSqliteStmt(DatabaseSql::sqlUpdateTrafficHour);
    SqliteStmt *stmtUpdateDay = getSqliteStmt(DatabaseSql::sqlUpdateTrafficDay);
    SqliteStmt *stmtUpdateMonth = getSqliteStmt(DatabaseSql::sqlUpdateTrafficMonth);

    SqliteStmt *stmtUpdateAppTotal = getSqliteStmt(DatabaseSql::sqlUpdateTrafficAppTotal);

    stmtUpdateAppHour->bindInt(1, unixHour);
    stmtUpdateAppDay->bindInt(1, unixDay);
    stmtUpdateAppMonth->bindInt(1, unixMonth);

    stmtUpdateHour->bindInt(1, unixHour);
    stmtUpdateDay->bindInt(1, unixDay);
    stmtUpdateMonth->bindInt(1, unixMonth);

    m_sqliteDb->beginTransaction();

    for (quint16 procIndex = 0; procIndex < procCount; ++procIndex) {
        const bool active = procBits[procIndex / 8] & (1 << (procIndex & 7));
        if (!active) {
            delProcIndexes.append(procIndex);
        }

        const quint32 *procTrafBytes = &trafBytes[procIndex * 2];
        const quint32 inBytes = procTrafBytes[0];
        const quint32 outBytes = procTrafBytes[1];

        if (!(isNewHour || inBytes || outBytes))
            continue;

        const qint64 appId = m_appIds.at(procIndex);

        // Insert zero bytes
        if (isNewHour) {
            insertTraffic(stmtInsertAppHour, appId);
            insertTraffic(stmtInsertHour);

            if (isNewDay) {
                insertTraffic(stmtInsertAppDay, appId);
                insertTraffic(stmtInsertDay);

                if (isNewMonth) {
                    insertTraffic(stmtInsertAppMonth, appId);
                    insertTraffic(stmtInsertMonth);
                }
            }
        }

        // Update bytes
        updateTraffic(stmtUpdateAppHour, inBytes, outBytes, appId);
        updateTraffic(stmtUpdateAppDay, inBytes, outBytes, appId);
        updateTraffic(stmtUpdateAppMonth, inBytes, outBytes, appId);

        updateTraffic(stmtUpdateHour, inBytes, outBytes);
        updateTraffic(stmtUpdateDay, inBytes, outBytes);
        updateTraffic(stmtUpdateMonth, inBytes, outBytes);

        // Update total bytes
        stmtUpdateAppTotal->bindInt64(1, appId);
        updateTraffic(stmtUpdateAppTotal, inBytes, outBytes);
    }

    m_sqliteDb->commitTransaction();

    // Delete inactive processes
    {
        int i = delProcIndexes.size();
        while (--i >= 0) {
            const quint16 procIndex = delProcIndexes.at(i);
            m_appIds.removeAt(procIndex);
        }
    }
}

bool DatabaseManager::createTables()
{
    m_sqliteDb->beginTransaction();

    const bool res = m_sqliteDb->execute(DatabaseSql::sqlCreateTables);

    m_sqliteDb->commitTransaction();

    return res;
}

qint64 DatabaseManager::getAppId(const QString &appPath, bool &isNew)
{
    qint64 appId = 0;

    // Check existing
    {
        SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlSelectAppId);

        stmt->bindText(1, appPath);
        if (stmt->step() == SqliteStmt::StepRow) {
            appId = stmt->columnInt64();
        }
        stmt->reset();
    }

    // Create new one
    if (!appId) {
        SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlInsertAppId);
        const qint64 unixTime = QDateTime::currentSecsSinceEpoch();

        stmt->bindText(1, appPath);
        stmt->bindInt64(2, unixTime);
        stmt->bindInt64(3, unixTime);

        if (stmt->step() == SqliteStmt::StepDone) {
            appId = m_sqliteDb->lastInsertRowid();
            isNew = true;
        }
        stmt->reset();
    }

    return appId;
}

void DatabaseManager::getAppList(QStringList &list)
{
    SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlSelectAppPaths);

    while (stmt->step() == SqliteStmt::StepRow) {
        list.append(stmt->columnText());
    }
    stmt->reset();
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

void DatabaseManager::insertTraffic(SqliteStmt *stmt, qint64 appId)
{
    if (appId != 0) {
        stmt->bindInt64(2, appId);
    }

    stmt->step();
    stmt->reset();
}

void DatabaseManager::updateTraffic(SqliteStmt *stmt, quint32 inBytes,
                                       quint32 outBytes, qint64 appId)
{
    stmt->bindInt64(2, inBytes);
    stmt->bindInt64(3, outBytes);

    if (appId != 0) {
        stmt->bindInt64(4, appId);
    }

    stmt->step();
    stmt->reset();
}

qint32 DatabaseManager::getUnixDay(qint64 unixTime)
{
    const QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();

    return qint32(QDateTime(date).toSecsSinceEpoch() / 3600);
}

qint32 DatabaseManager::getUnixMonth(qint64 unixTime)
{
    const QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();

    return qint32(QDateTime(QDate(date.year(), date.month(), 1))
                  .toSecsSinceEpoch() / 3600);
}
