#ifndef DBUTIL_H
#define DBUTIL_H

#include <QObject>
#include <QVector>

#include "sqlite_types.h"
#include "sqlitestmt.h"

class DbUtil
{
public:
    static void doList(const SqliteStmtList &stmtList);

    /* The rows' first column; resets the statement */
    template<typename T = qint64>
    static QVector<T> stepIds(SqliteStmt *stmt)
    {
        QVector<T> ids;

        while (stmt->step() == SqliteStmt::StepRow) {
            ids.append(T(stmt->columnInt64(0)));
        }

        stmt->reset();

        return ids;
    }
};

#endif // DBUTIL_H
