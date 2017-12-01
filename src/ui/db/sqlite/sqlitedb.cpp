#include "sqlitedb.h"

#include "sqlite3.h"

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

qint64 SqliteDb::lastInsertRowid()
{
    return sqlite3_last_insert_rowid(m_db);
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

QString SqliteDb::errorMessage() const
{
    const char *text = sqlite3_errmsg(m_db);

    return QString::fromUtf8(text);
}
