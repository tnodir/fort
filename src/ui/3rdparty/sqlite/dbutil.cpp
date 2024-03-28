#include "dbutil.h"

#include "sqlitestmt.h"

void DbUtil::doList(const SqliteStmtList &stmtList)
{
    for (SqliteStmt *stmt : stmtList) {
        const auto stepRes = stmt->step();
        Q_UNUSED(stepRes);

        stmt->reset();
    }
}
