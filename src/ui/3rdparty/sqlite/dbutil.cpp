#include "dbutil.h"

#include "sqlitedb.h"
#include "sqlitestmt.h"

DbUtil::DbUtil(SqliteDb *sqliteDb, bool *ok) : m_sqliteDb(sqliteDb), m_ok(ok) { }

DbUtil &DbUtil::sql(const QString &sql)
{
    m_sqlUtf8 = sql.toUtf8();

    return this->sql(m_sqlUtf8.constData());
}

DbUtil &DbUtil::sql(const char *sql)
{
    m_sql = sql;

    return *this;
}

DbUtil &DbUtil::vars(const QVariantList &vars)
{
    m_vars = vars;

    return *this;
}

DbUtil &DbUtil::vars(const QVariantHash &varsMap)
{
    m_varsMap = varsMap;

    return *this;
}

bool DbUtil::prepare(SqliteStmt &stmt)
{
    if (!stmt.prepare(sqliteDb()->db(), m_sql))
        return false;

    return stmt.bindVars(m_vars) && stmt.bindVarsMap(m_varsMap);
}

bool DbUtil::prepareRow(SqliteStmt &stmt)
{
    return prepare(stmt) && stmt.step() == SqliteStmt::StepRow;
}

QVariant DbUtil::execute(int resultCount, bool &ok)
{
    QVariantList list;

    SqliteStmt stmt;
    bool success = false;

    if (prepare(stmt)) {
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

    ok = success;

    const int listSize = list.size();
    return (listSize == 0) ? QVariant() : (listSize == 1 ? list.at(0) : list);
}

QVariant DbUtil::execute(int resultCount)
{
    bool ok = false;
    const auto res = execute(resultCount, ok);
    setResult(ok);
    return res;
}

bool DbUtil::executeOk()
{
    bool ok = false;
    execute(/*resultCount=*/0, ok);
    setResult(ok);
    return ok;
}

int DbUtil::getFreeId(int minId, int maxId)
{
    int resId = minId;

    SqliteStmt stmt;
    if (prepare(stmt)) {
        while (stmt.step() == SqliteStmt::StepRow) {
            const int id = stmt.columnInt(0);

            if (resId < id || id >= maxId)
                break;

            resId = id + 1;
        }
    }

    setResult(resId < maxId);

    return resId;
}

void DbUtil::setResult(bool v)
{
    if (m_ok) {
        *m_ok = v;
    }
}
