#include "sqlitestmt.h"

#include "sqlite3.h"

SqliteStmt::SqliteStmt() :
    m_stmt(nullptr)
{
}

SqliteStmt::~SqliteStmt()
{
    finalize();
}

bool SqliteStmt::prepare(sqlite3 *db, const char *sql,
                         SqliteStmt::PrepareFlags flags)
{
    return sqlite3_prepare_v3(db, sql, -1, flags,
                              &m_stmt, nullptr) == SQLITE_OK;
}

void SqliteStmt::finalize()
{
    if (m_stmt != nullptr) {
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
}

bool SqliteStmt::bindInt(int index, qint32 number)
{
    return sqlite3_bind_int(m_stmt, index, number) == SQLITE_OK;
}

bool SqliteStmt::bindInt64(int index, qint64 number)
{
    return sqlite3_bind_int64(m_stmt, index, number) == SQLITE_OK;
}

bool SqliteStmt::bindNull(int index)
{
    return sqlite3_bind_null(m_stmt, index) == SQLITE_OK;
}

bool SqliteStmt::bindText(int index, const QString &text)
{
    const int bytesCount = text.size() * int(sizeof(wchar_t));

    m_bindObjects.insert(index, text);

    return sqlite3_bind_text16(m_stmt, index, text.utf16(),
                               bytesCount, SQLITE_STATIC) == SQLITE_OK;
}

bool SqliteStmt::clearBindings()
{
    m_bindObjects.clear();

    return sqlite3_clear_bindings(m_stmt) == SQLITE_OK;
}

bool SqliteStmt::reset()
{
    return sqlite3_reset(m_stmt) == SQLITE_OK;
}

bool SqliteStmt::isBusy()
{
    return sqlite3_stmt_busy(m_stmt) != 0;
}

SqliteStmt::StepResult SqliteStmt::step()
{
    const int res = sqlite3_step(m_stmt);

    switch (res) {
    case SQLITE_ROW:
        return StepRow;
    case SQLITE_DONE:
        return StepDone;
    default:
        return StepError;
    }
}

int SqliteStmt::dataCount()
{
    return sqlite3_data_count(m_stmt);
}

qint32 SqliteStmt::columnInt(int column)
{
    return sqlite3_column_int(m_stmt, column);
}

qint64 SqliteStmt::columnInt64(int column)
{
    return sqlite3_column_int64(m_stmt, column);
}

QString SqliteStmt::columnText(int column)
{
    const int bytesCount = sqlite3_column_bytes16(m_stmt, column);
    if (!bytesCount)
        return QString();

    const ushort *p = static_cast<const ushort *>(
                sqlite3_column_text16(m_stmt, column));

    return QString::fromUtf16(p, bytesCount / int(sizeof(wchar_t)));
}
