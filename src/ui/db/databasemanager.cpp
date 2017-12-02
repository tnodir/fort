#include "databasemanager.h"

#include <QDateTime>

#include "../util/fileutil.h"
#include "sqlite/sqliteengine.h"
#include "sqlite/sqlitedb.h"
#include "sqlite/sqlitestmt.h"

static const char * const sqlPragmas =
        "PRAGMA locking_mode=EXCLUSIVE;"
        "PRAGMA journal_mode=WAL;"
        "PRAGMA synchronous=NORMAL;"
        ;

static const char * const sqlCreateTables =
        "CREATE TABLE app("
        "  id INTEGER PRIMARY KEY,"
        "  path TEXT UNIQUE NOT NULL"
        ");"

        "CREATE TABLE traffic_app_hour("
        "  app_id INTEGER NOT NULL,"
        "  unix_hour INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL,"
        "  PRIMARY KEY (app_id, unix_hour)"
        ") WITHOUT ROWID;"
        ;

static const char * const sqlSelectAppId =
        "SELECT id FROM app WHERE path = ?1;"
        ;

static const char * const sqlInsertAppId =
        "INSERT INTO app(path) VALUES(?1);"
        ;

static const char * const sqlUpsertAppTraffic =
        "WITH new(app_id, unix_hour, in_bytes, out_bytes)"
        "    AS ( VALUES(?1, ?2, ?3, ?4) )"
        "  INSERT OR REPLACE INTO traffic_app_hour("
        "    app_id, unix_hour, in_bytes, out_bytes)"
        "  SELECT new.app_id, new.unix_hour,"
        "    new.in_bytes + ifnull(old.in_bytes, 0),"
        "    new.out_bytes + ifnull(old.out_bytes, 0)"
        "  FROM new LEFT JOIN traffic_app_hour AS old"
        "    ON new.app_id = old.app_id"
        "      AND new.unix_hour = old.unix_hour;"
        ;

DatabaseManager::DatabaseManager(const QString &filePath,
                                 QObject *parent) :
    QObject(parent),
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

    m_sqliteDb->execute(sqlPragmas);

    return fileExists || createTables();
}

void DatabaseManager::handleProcNew(const QString &appPath)
{
    const qint64 appId = getAppId(appPath);

    m_appPaths.append(appPath);
    m_appIds.append(appId);
}

void DatabaseManager::handleStatTraf(quint16 procCount, const quint8 *procBits,
                                     const quint32 *trafBytes)
{
    QVector<quint16> delProcIndexes;

    const qint32 unixHour = qint32(QDateTime::currentSecsSinceEpoch() / 3600);

    SqliteStmt *stmtUpsert = getSqliteStmt(sqlUpsertAppTraffic);

    stmtUpsert->bindInt(2, unixHour);

    m_sqliteDb->beginTransaction();

    for (quint16 procIndex = 0; procIndex < procCount; ++procIndex) {
        const bool active = procBits[procIndex / 8] & (1 << (procIndex & 7));
        if (!active) {
            delProcIndexes.append(procIndex);
        }

        const quint32 *procTrafBytes = &trafBytes[procIndex * 2];
        const quint32 inBytes = procTrafBytes[0];
        const quint32 outBytes = procTrafBytes[1];

        if (inBytes != 0 || outBytes != 0) {
            const qint64 appId = m_appIds.at(procIndex);

            stmtUpsert->bindInt64(1, appId);
            stmtUpsert->bindInt64(3, inBytes);
            stmtUpsert->bindInt64(4, outBytes);

            stmtUpsert->step();
            stmtUpsert->reset();
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

bool DatabaseManager::createTables()
{
    return m_sqliteDb->execute(sqlCreateTables);
}

qint64 DatabaseManager::getAppId(const QString &appPath)
{
    qint64 appId = 0;

    // Check existing
    {
        SqliteStmt *stmt = getSqliteStmt(sqlSelectAppId);

        stmt->bindText(1, appPath);
        if (stmt->step() == SqliteStmt::StepRow) {
            appId = stmt->columnInt64();
        }
        stmt->reset();
    }

    // Create new one
    if (!appId) {
        SqliteStmt *stmt = getSqliteStmt(sqlInsertAppId);

        stmt->bindText(1, appPath);
        if (stmt->step() == SqliteStmt::StepDone) {
            appId = m_sqliteDb->lastInsertRowid();
        }
        stmt->reset();
    }

    return appId;
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
