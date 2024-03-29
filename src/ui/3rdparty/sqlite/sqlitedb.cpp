#include "sqlitedb.h"

#include <QAtomicInt>
#include <QDateTime>
#include <QDir>
#include <QLoggingCategory>
#include <QSet>

#include <sqlite.h>

#include "dbquery.h"
#include "sqlitestmt.h"

namespace {

const QLoggingCategory LC("db");

constexpr int DATABASE_BUSY_TIMEOUT = 3000; // 3 seconds

const char *const defaultSqlPragmas = "PRAGMA journal_mode = WAL;"
                                      "PRAGMA locking_mode = NORMAL;"
                                      "PRAGMA synchronous = NORMAL;"
                                      "PRAGMA encoding = 'UTF-8';";

const QString ftsTableSuffix = "_fts";

QAtomicInt g_sqliteInitCount;

bool removeDbFile(const QString &filePath)
{
    if (filePath.startsWith(QLatin1Char(':')))
        return true; // can't remove from qrc

    if (QFile::exists(filePath) && !QFile::remove(filePath)) {
        qCCritical(LC) << "Cannot remove file:" << filePath;
        return false;
    }

    return true;
}

bool renameDbFile(const QString &filePath, const QString &newFilePath)
{
    removeDbFile(newFilePath);

    if (!QFile::rename(filePath, newFilePath)) {
        qCCritical(LC) << "Cannot rename file" << filePath << "to" << newFilePath;
        return false;
    }
    return true;
}

QString makeTriggerColumnNames(
        const QString &rowIdName, const QStringList &columnNames, const QString &prefix)
{
    return (prefix + rowIdName) + (',' + prefix) + columnNames.join(',' + prefix);
}

}

SqliteDb::SqliteDb(const QString &filePath, quint32 openFlags) :
    m_openFlags(openFlags != 0 ? openFlags : OpenDefaultReadWrite), m_filePath(filePath)
{
    if (g_sqliteInitCount++ == 0) {
        sqlite3_initialize();
    }
}

SqliteDb::~SqliteDb()
{
    close();

    if (--g_sqliteInitCount == 0) {
        sqlite3_shutdown();
    }
}

bool SqliteDb::open()
{
    const auto filePathUtf8 = m_filePath.toUtf8();

    return sqlite3_open_v2(filePathUtf8.data(), &m_db, m_openFlags, nullptr) == SQLITE_OK;
}

void SqliteDb::close()
{
    clearStmts();

    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SqliteDb::attach(const QString &schemaName, const QString &filePath)
{
    return DbQuery(this)
            .sql("ATTACH DATABASE ?1 AS ?2;")
            .vars({ filePath, schemaName })
            .executeOk();
}

bool SqliteDb::detach(const QString &schemaName)
{
    return DbQuery(this).sql("DETACH DATABASE ?1;").vars({ schemaName }).executeOk();
}

bool SqliteDb::vacuum()
{
    return execute("VACUUM;");
}

bool SqliteDb::vacuumInto(const QString &filePath)
{
    return DbQuery(this).sql("VACUUM INTO ?1;").vars({ filePath }).executeOk();
}

bool SqliteDb::execute(const char *sql)
{
    return sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool SqliteDb::executeStr(const QString &sql)
{
    const auto sqlUtf8 = sql.toUtf8();

    return execute(sqlUtf8.data());
}

bool SqliteDb::done(SqliteStmt *stmt)
{
    const SqliteStmt::StepResult res = stmt->step();
    const bool ok = (res == SqliteStmt::StepDone && changes() != 0);
    stmt->reset();
    return ok;
}

qint64 SqliteDb::lastInsertRowid() const
{
    return sqlite3_last_insert_rowid(m_db);
}

int SqliteDb::changes() const
{
    return sqlite3_changes(m_db);
}

bool SqliteDb::beginTransaction()
{
    return execute("BEGIN;");
}

bool SqliteDb::beginWriteTransaction()
{
    return execute("BEGIN IMMEDIATE;");
}

bool SqliteDb::endTransaction(bool ok)
{
    if (ok) {
        ok = commitTransaction();
    } else {
        rollbackTransaction();
    }
    return ok;
}

bool SqliteDb::commitTransaction()
{
    return execute("COMMIT;");
}

bool SqliteDb::rollbackTransaction()
{
    return execute("ROLLBACK;");
}

bool SqliteDb::beginSavepoint(const char *name)
{
    return !name ? execute("SAVEPOINT _;") : executeStr(QString("SAVEPOINT %1;").arg(name));
}

bool SqliteDb::releaseSavepoint(const char *name)
{
    return !name ? execute("RELEASE _;") : executeStr(QString("RELEASE %1;").arg(name));
}

bool SqliteDb::rollbackSavepoint(const char *name)
{
    return !name ? execute("ROLLBACK TO _;") : executeStr(QString("ROLLBACK TO %1;").arg(name));
}

int SqliteDb::errorCode() const
{
    return sqlite3_errcode(m_db);
}

QString SqliteDb::errorMessage() const
{
    const char *text = sqlite3_errmsg(m_db);

    return QString::fromUtf8(text);
}

int SqliteDb::userVersion()
{
    return DbQuery(this).sql("PRAGMA user_version;").execute().toInt();
}

bool SqliteDb::setUserVersion(int v)
{
    const auto sql = QString("PRAGMA user_version = %1;").arg(v);
    return executeStr(sql);
}

QString SqliteDb::encoding()
{
    return DbQuery(this).sql("PRAGMA encoding;").execute().toString();
}

bool SqliteDb::setEncoding(const QString &v)
{
    const auto sql = QString("PRAGMA encoding = '%1';").arg(v);
    return executeStr(sql);
}

bool SqliteDb::setBusyTimeoutMs(int v)
{
    return sqlite3_busy_timeout(m_db, v) == SQLITE_OK;
}

QString SqliteDb::getFtsTableName(const QString &tableName)
{
    return tableName + ftsTableSuffix;
}

QString SqliteDb::migrationOldSchemaName()
{
    return QLatin1String("old");
}

QString SqliteDb::migrationNewSchemaName()
{
    return QLatin1String("main");
}

QString SqliteDb::entityName(const QString &schemaName, const QString &objectName)
{
    return schemaName.isEmpty() ? objectName : schemaName + '.' + objectName;
}

QStringList SqliteDb::tableNames(const QString &schemaName)
{
    QStringList list;

    const auto masterTable = entityName(schemaName, "sqlite_master");
    const auto sql = QString("SELECT name FROM %1"
                             "  WHERE type = 'table'"
                             "    AND name NOT LIKE 'sqlite_%'"
                             "    AND name NOT LIKE '%%2%';")
                             .arg(masterTable, ftsTableSuffix);

    SqliteStmt stmt;
    if (stmt.prepare(db(), sql.toLatin1())) {
        while (stmt.step() == SqliteStmt::StepRow) {
            list.append(stmt.columnText());
        }
    }

    return list;
}

QStringList SqliteDb::columnNames(const QString &tableName, const QString &schemaName)
{
    QStringList list;

    const auto schemaTableName = entityName(schemaName, tableName);
    const auto sql = QString("SELECT * FROM %1 WHERE 0 = 1 LIMIT 1;").arg(schemaTableName);

    SqliteStmt stmt;
    if (stmt.prepare(db(), sql.toLatin1())) {
        const int n = stmt.columnCount();
        for (int i = 0; i < n; ++i) {
            list.append(stmt.columnName(i));
        }
    }

    return list;
}

bool SqliteDb::migrate(MigrateOptions &opt)
{
    setBusyTimeoutMs(DATABASE_BUSY_TIMEOUT);

    if (!opt.sqlPragmas) {
        opt.sqlPragmas = defaultSqlPragmas;
    }
    execute(opt.sqlPragmas);

    // Check version
    const int userVersion = this->userVersion();
    if (userVersion == opt.version)
        return true;

    opt.userVersion = userVersion;

    // Check migration options
    if (!canMigrate(opt))
        return false;

    // Migrate the DB
    const bool isNewDb = (userVersion == 0);
    if (isNewDb) {
        opt.recreate = false;
    }

    return migrateDb(opt, userVersion, isNewDb);
}

bool SqliteDb::canMigrate(const MigrateOptions &opt) const
{
    if (opt.userVersion > opt.version) {
        qCWarning(LC) << "Cannot open new DB" << opt.userVersion << "from old application"
                      << opt.version;
        return false;
    }

    if ((openFlags() & OpenReadOnly) != 0) {
        qCWarning(LC) << "Cannot migrate a read-only DB";
        return false;
    }

    return true;
}

bool SqliteDb::migrateDb(const MigrateOptions &opt, int userVersion, bool isNewDb)
{
    if (!migrateDbBegin(opt, userVersion, isNewDb))
        return false;

    // Run migration SQL scripts
    if (!migrateSqlScripts(opt, userVersion, isNewDb))
        return false;

    return migrateDbEnd(opt);
}

bool SqliteDb::migrateSqlScripts(const MigrateOptions &opt, int userVersion, bool isNewDb)
{
    const QDir sqlDir(opt.sqlDir);
    bool ok = true;

    beginWriteTransaction();

    while (userVersion < opt.version) {
        ++userVersion;

        beginSavepoint();

        ok = migrateSqlScript(sqlDir, userVersion);

        if (ok && opt.migrateFunc) {
            ok = opt.migrateFunc(this, userVersion, isNewDb, opt.migrateContext);
        }

        if (!ok) {
            qCCritical(LC) << "Migration error to version:" << userVersion << errorMessage();
            rollbackSavepoint();
            break;
        }

        releaseSavepoint();
    }

    this->setUserVersion(userVersion);

    commitTransaction();

    return ok;
}

bool SqliteDb::migrateSqlScript(const QDir &sqlDir, int userVersion)
{
    const QString filePath = sqlDir.filePath(QString("%1.sql").arg(userVersion));

    QFile file(filePath);
    if (!file.exists())
        return true;

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCWarning(LC) << "Cannot open migration file" << filePath << file.errorString();
        return false;
    }

    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        qCWarning(LC) << "Migration file is empty" << filePath;
        return false;
    }

    return execute(data.constData());
}

bool SqliteDb::migrateDbBegin(const MigrateOptions &opt, int &userVersion, bool &isNewDb)
{
    // Re-create the DB
    if (!opt.recreate)
        return true;

    userVersion = 0;
    isNewDb = true;

    return clearWithBackup(opt.sqlPragmas);
}

bool SqliteDb::migrateDbEnd(const MigrateOptions &opt)
{
    if (!createFtsTables(opt))
        return false;

    // Re-create the DB: End
    if (!opt.recreate)
        return true;

    return importBackup(opt);
}

bool SqliteDb::createFtsTables(const MigrateOptions &opt)
{
    if (opt.ftsTables.isEmpty())
        return true;

    bool success = true;

    beginWriteTransaction();

    for (const FtsTable &ftsTable : opt.ftsTables) {
        beginSavepoint();

        success = createFtsTable(ftsTable);

        if (success) {
            releaseSavepoint();
        } else {
            qCCritical(LC) << "FTS error:" << ftsTable.contentTable << errorMessage();
            rollbackSavepoint();
            break;
        }
    }

    commitTransaction();

    return success;
}

bool SqliteDb::createFtsTable(const FtsTable &ftsTable)
{
    /*
     * %1: content table name
     * %2: fts table name
     * %3: content rowid column name
     * %4: content column names list
     * %5: triggered new column names list (new.*)
     * %6: triggered old column names list (old.*)
     */
    static const char *const ftsCreateSql =
            "CREATE VIRTUAL TABLE IF NOT EXISTS %2"
            "  USING fts5(%4, content='%1', content_rowid='%3');"
            "CREATE TRIGGER IF NOT EXISTS %2_ai AFTER INSERT ON %1 BEGIN"
            "  INSERT INTO %2(rowid, %4) VALUES (%5);"
            "END;"
            "CREATE TRIGGER IF NOT EXISTS %2_ad AFTER DELETE ON %1 BEGIN"
            "  INSERT INTO %2(%2, rowid, %4) VALUES('delete', %6);"
            "END;"
            "CREATE TRIGGER IF NOT EXISTS %2_au AFTER UPDATE ON %1 BEGIN"
            "  INSERT INTO %2(%2, rowid, %4) VALUES('delete', %6);"
            "  INSERT INTO %2(rowid, %4) VALUES (%5);"
            "END;";

    const auto &contentTableName = ftsTable.contentTable;
    const auto ftsTableName = getFtsTableName(contentTableName);
    const auto &contentRowidName = ftsTable.contentRowid;
    const auto contentColumnNames = ftsTable.columns.join(',');
    const auto newTriggerColumnNames =
            makeTriggerColumnNames(contentRowidName, ftsTable.columns, "new.");
    const auto oldTriggerColumnNames =
            makeTriggerColumnNames(contentRowidName, ftsTable.columns, "old.");

    const auto sql =
            QString(ftsCreateSql)
                    .arg(contentTableName, ftsTableName, contentRowidName, contentColumnNames,
                            newTriggerColumnNames, oldTriggerColumnNames);

    return executeStr(sql);
}

bool SqliteDb::clearWithBackup(const char *sqlPragmas)
{
    const QString oldEncoding = this->encoding();

    close();

    const QString tempFilePath = backupFilePath();

    if (!(renameDbFile(m_filePath, tempFilePath) && open())) {
        qCWarning(LC) << "Cannot re-create the DB" << m_filePath;
        renameDbFile(tempFilePath, m_filePath);
        return false;
    }

    execute(sqlPragmas);
    setEncoding(oldEncoding);

    return true;
}

bool SqliteDb::importBackup(const MigrateOptions &opt)
{
    bool success = true;

    const QString tempFilePath = backupFilePath();

    // Re-import the DB
    if (opt.importOldData) {
        success = importDb(opt, tempFilePath);
    }

    // Remove the old DB
    if (success) {
        removeDbFile(tempFilePath);
    }

    return success;
}

QString SqliteDb::backupFilePath() const
{
    return m_filePath + ".temp";
}

bool SqliteDb::importDb(const MigrateOptions &opt, const QString &sourceFilePath)
{
    const QString srcSchema = migrationOldSchemaName();
    const QString dstSchema = migrationNewSchemaName();

    if (!attach(srcSchema, sourceFilePath)) {
        qCWarning(LC) << "Cannot attach the DB" << sourceFilePath << "Error:" << errorMessage();
        return false;
    }

    // Import Data
    bool success = true;

    beginWriteTransaction();

    // Copy tables
    if (opt.autoCopyTables) {
        success = copyTables(srcSchema, dstSchema);
    }

    // Migrate
    if (success && opt.migrateFunc) {
        success = opt.migrateFunc(this, opt.userVersion, /*isNewDb=*/false, opt.migrateContext);
    }

    endTransaction(success);

    detach(srcSchema);

    return success;
}

bool SqliteDb::copyTables(const QString &srcSchema, const QString &dstSchema)
{
    const QStringList srcTableNames = tableNames(srcSchema);

    for (const QString &tableName : srcTableNames) {
        if (!copyTable(srcSchema, dstSchema, tableName))
            return false;
    }

    return true;
}

bool SqliteDb::copyTable(
        const QString &srcSchema, const QString &dstSchema, const QString &tableName)
{
    const QStringList dstColumns = this->columnNames(tableName, dstSchema);
    if (dstColumns.isEmpty()) {
        return true; // new schema doesn't contain old table
    }

    const QStringList srcColumns = this->columnNames(tableName, srcSchema);
    if (srcColumns.isEmpty()) {
        return true; // empty old table
    }

    // Intersect column names
    auto columnsSet = QSet<QString>(srcColumns.constBegin(), srcColumns.constEnd());
    const auto dstColumnsSet = QSet<QString>(dstColumns.constBegin(), dstColumns.constEnd());
    columnsSet.intersect(dstColumnsSet);

    const QStringList columns(columnsSet.constBegin(), columnsSet.constEnd());
    const QString columnNames = columns.join(", ");

    // Insert
    const auto sql = QString("INSERT INTO %1 (%3) SELECT %3 FROM %2;")
                             .arg(entityName(dstSchema, tableName),
                                     entityName(srcSchema, tableName), columnNames);

    return executeStr(sql);
}

SqliteStmt *SqliteDb::stmt(const char *sql)
{
    SqliteStmt *stmt = m_stmts.value(sql);

    if (!stmt) {
        stmt = new SqliteStmt();
        stmt->prepare(db(), sql, SqliteStmt::PreparePersistent);

        m_stmts.insert(sql, stmt);
    }

    return stmt;
}

void SqliteDb::clearStmts()
{
    qDeleteAll(m_stmts);
    m_stmts.clear();
}

bool SqliteDb::isIoError(int errCode)
{
    return qint8(errCode) == SQLITE_IOERR;
}

bool SqliteDb::isDebugError(int errCode)
{
    return qint8(errCode) == SQLITE_SCHEMA;
}

bool SqliteDb::setErrorLogCallback(SQLITEDB_ERRORLOG_FUNC errorLogFunc, void *context)
{
    return sqlite3_config(SQLITE_CONFIG_LOG, errorLogFunc, context) == SQLITE_OK;
}
