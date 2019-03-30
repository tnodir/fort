#include "sqlitedb.h"

#include <QDir>

#include <sqlite3.h>

SqliteDb::SqliteDb() :
    m_db(nullptr)
{
}

SqliteDb::~SqliteDb()
{
    close();
}

bool SqliteDb::open(const QString &filePath)
{
    return sqlite3_open16(filePath.utf16(), &m_db) == SQLITE_OK;
}

void SqliteDb::close()
{
    if (m_db != nullptr) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SqliteDb::execute(const char *sql)
{
    return sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool SqliteDb::execute16(const ushort *sql)
{
    sqlite3_stmt *stmt = nullptr;
    int res = sqlite3_prepare16_v2(db(), sql, -1, &stmt, nullptr);
    if (stmt != nullptr) {
        res = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return res;
}

bool SqliteDb::executeStr(const QString &sql)
{
    return execute16(sql.utf16());
}

QVariant SqliteDb::executeOut(const char *sql)
{
    QVariant res;
    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr);
    if (stmt != nullptr) {
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            switch (sqlite3_column_type(stmt, 0)) {
            case SQLITE_INTEGER:
                res = sqlite3_column_int64(stmt, 0);
                break;
            case SQLITE_FLOAT:
                res = sqlite3_column_double(stmt, 0);
                break;
            case SQLITE_TEXT:
                res = QString::fromUtf8(reinterpret_cast<const char *>(
                                            sqlite3_column_text(stmt, 0)));
                break;
            default:
                Q_UNREACHABLE();
            }
        }
        sqlite3_finalize(stmt);
    }
    return res;
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
    return (name == nullptr)
            ? execute("SAVEPOINT _;")
            : executeStr(QString("SAVEPOINT %1;").arg(name));
}

bool SqliteDb::releaseSavepoint(const char *name)
{
    return (name == nullptr)
            ? execute("RELEASE _;")
            : executeStr(QString("RELEASE %1;").arg(name));
}

bool SqliteDb::rollbackSavepoint(const char *name)
{
    return (name == nullptr)
            ? execute("ROLLBACK TO _;")
            : executeStr(QString("ROLLBACK TO %1;").arg(name));
}

QString SqliteDb::errorMessage() const
{
    const char *text = sqlite3_errmsg(m_db);

    return QString::fromUtf8(text);
}

bool SqliteDb::migrate(const QString &sqlDir, int version)
{
    // Check version
    const int userVersion = executeOut("PRAGMA user_version;").toInt();
    if (userVersion == version)
        return true;

    // Run migration SQL scripts
    QDir dir(sqlDir);
    bool res = true;

    beginTransaction();
    for (int i = userVersion + 1; i <= version; ++i) {
        const QString filePath = dir.filePath(QString::number(i) + ".sql");

        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text))
            continue;

        const QByteArray data = file.readAll();
        if (data.isEmpty())
            continue;

        beginSavepoint();
        if (!execute(data.constData())) {
            res = false;
            rollbackSavepoint();
            break;
        }
        releaseSavepoint();
    }
    commitTransaction();

    return res;
}
