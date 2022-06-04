#include "sqlitedb.h"

#include <QAtomicInt>
#include <QDateTime>
#include <QDir>
#include <QLoggingCategory>
#include <QSet>

#include <sqlite.h>

#include "sqlitestmt.h"

namespace {

const QLoggingCategory LC("db");

const char *const defaultSqlPragmas = "PRAGMA journal_mode = WAL;"
                                      "PRAGMA locking_mode = NORMAL;"
                                      "PRAGMA synchronous = NORMAL;"
                                      "PRAGMA encoding = 'UTF-8';";

QAtomicInt g_sqliteInitCount;

bool removeDbFile(const QString &filePath)
{
    if (!filePath.startsWith(QLatin1Char(':')) && QFile::exists(filePath)
            && !QFile::remove(filePath)) {
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
    bool ok = false;
    executeEx("ATTACH DATABASE ?1 AS ?2;", { filePath, schemaName }, 0, &ok);
    return ok;
}

bool SqliteDb::detach(const QString &schemaName)
{
    bool ok = false;
    executeEx("DETACH DATABASE ?1;", { schemaName }, 0, &ok);
    return ok;
}

bool SqliteDb::vacuum()
{
    return execute("VACUUM;");
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

QVariant SqliteDb::executeEx(const char *sql, const QVariantList &vars, int resultCount, bool *ok)
{
    QVariantList list;

    SqliteStmt stmt;
    bool success = false;

    if (prepare(stmt, sql, vars)) {
        const auto stepRes = stmt.step();
        success = (stepRes != SqliteStmt::StepError);

        // Get result
        if (stepRes == SqliteStmt::StepRow) {
            for (int i = 0; i < resultCount; ++i) {
                const QVariant v = stmt.columnVar(i);
                list.append(v);
            }
        }
    }

    if (ok) {
        *ok = success;
    }

    const int listSize = list.size();
    return (listSize == 0) ? QVariant() : (listSize == 1 ? list.at(0) : list);
}

bool SqliteDb::prepare(SqliteStmt &stmt, const char *sql, const QVariantList &vars)
{
    return stmt.prepare(db(), sql) && (vars.isEmpty() || stmt.bindVars(vars));
}

bool SqliteDb::prepare(SqliteStmt &stmt, const QString &sql, const QVariantList &vars)
{
    const auto sqlData = sql.toUtf8();
    return prepare(stmt, sqlData.constData(), vars);
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

bool SqliteDb::endTransaction(bool ok)
{
    return ok ? commitTransaction() : rollbackTransaction();
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
    return executeEx("PRAGMA user_version;").toInt();
}

bool SqliteDb::setUserVersion(int v)
{
    const auto sql = QString("PRAGMA user_version = %1;").arg(v);
    return executeStr(sql);
}

QString SqliteDb::encoding()
{
    return executeEx("PRAGMA encoding;").toString();
}

bool SqliteDb::setEncoding(const QString &v)
{
    const auto sql = QString("PRAGMA encoding = '%1';").arg(v);
    return executeStr(sql);
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
                             "  WHERE type = 'table' AND name NOT LIKE 'sqlite_%';")
                             .arg(masterTable);

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
    if (!opt.sqlPragmas) {
        opt.sqlPragmas = defaultSqlPragmas;
    }
    execute(opt.sqlPragmas);

    // Check version
    int userVersion = this->userVersion();
    if (userVersion == opt.version)
        return true;

    if (userVersion > opt.version) {
        qCWarning(LC) << "Cannot open new DB" << userVersion << "from old application"
                      << opt.version;
        return false;
    }

    if ((openFlags() & OpenReadOnly) != 0) {
        qCWarning(LC) << "Cannot migrate a read-only DB";
        return false;
    }

    bool isNewDb = (userVersion == 0);
    if (isNewDb) {
        opt.recreate = false;
    }

    // Re-create the DB
    if (opt.recreate) {
        if (!clearWithBackup(opt.sqlPragmas))
            return false;

        userVersion = 0;
        isNewDb = true;
    }

    // Run migration SQL scripts
    bool success = migrateSqlScripts(opt, userVersion, isNewDb);

    // Re-create the DB: End
    if (success && opt.recreate) {
        success = importBackup(opt.importOldData, opt.migrateFunc, opt.migrateContext);
    }

    return success;
}

bool SqliteDb::migrateSqlScripts(const MigrateOptions &opt, int userVersion, bool isNewDb)
{
    const QDir dir(opt.sqlDir);
    bool success = true;

    beginTransaction();

    while (userVersion < opt.version) {
        ++userVersion;

        const QString filePath = dir.filePath(QString("%1.sql").arg(userVersion));

        beginSavepoint();

        QFile file(filePath);
        if (file.exists()) {
            if (!file.open(QFile::ReadOnly | QFile::Text)) {
                qCWarning(LC) << "Cannot open migration file" << filePath << file.errorString();
                success = false;
                break;
            }

            const QByteArray data = file.readAll();
            if (data.isEmpty()) {
                qCWarning(LC) << "Migration file is empty" << filePath;
                success = false;
                break;
            }

            success = execute(data.constData());
        }

        if (success && opt.migrateFunc) {
            success = opt.migrateFunc(this, userVersion, isNewDb, opt.migrateContext);
        }

        if (success) {
            releaseSavepoint();
        } else {
            qCCritical(LC) << "Migration error:" << filePath << errorMessage();
            rollbackSavepoint();
            break;
        }
    }

    this->setUserVersion(userVersion);

    commitTransaction();

    return success;
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

bool SqliteDb::importBackup(
        bool importOldData, SQLITEDB_MIGRATE_FUNC migrateFunc, void *migrateContext)
{
    bool success = true;

    const QString tempFilePath = backupFilePath();

    // Re-import the DB
    if (importOldData) {
        success = importDb(tempFilePath, migrateFunc, migrateContext);
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

bool SqliteDb::importDb(
        const QString &sourceFilePath, SQLITEDB_MIGRATE_FUNC migrateFunc, void *migrateContext)
{
    const QString srcSchema = migrationOldSchemaName();
    const QString dstSchema = migrationNewSchemaName();

    if (!attach(srcSchema, sourceFilePath)) {
        qCWarning(LC) << "Cannot attach the DB" << sourceFilePath << "Error:" << errorMessage();
        return false;
    }

    // Import Data
    bool success = true;

    beginTransaction();

    const QStringList srcTableNames = tableNames(srcSchema);

    // Copy tables
    for (const QString &tableName : srcTableNames) {
        if (!copyTable(srcSchema, dstSchema, tableName)) {
            success = false;
            break;
        }
    }

    // Migrate
    if (success && migrateFunc) {
        success = migrateFunc(this, userVersion(), false, migrateContext);
    }

    endTransaction(success);

    detach(srcSchema);

    return success;
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto columnsSet = QSet<QString>(srcColumns.constBegin(), srcColumns.constEnd());
    const auto dstColumnsSet = QSet<QString>(dstColumns.constBegin(), dstColumns.constEnd());
#else
    auto columnsSet = QSet<QString>::fromList(srcColumns);
    const auto dstColumnsSet = QSet<QString>::fromList(dstColumns);
#endif
    columnsSet.intersect(dstColumnsSet);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const QStringList columns(columnsSet.constBegin(), columnsSet.constEnd());
#else
    const QStringList columns(columnsSet.toList());
#endif
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
