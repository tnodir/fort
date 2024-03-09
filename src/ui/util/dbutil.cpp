#include "dbutil.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

int DbUtil::getFreeId(SqliteDb *sqliteDb, const char *sqlSelectIds, int minId, int maxId, bool &ok)
{
    SqliteStmt stmt;
    if (stmt.prepare(sqliteDb->db(), sqlSelectIds)) {
        stmt.bindInt(1, minId);

        while (stmt.step() == SqliteStmt::StepRow) {
            const int id = stmt.columnInt(0);
            if (minId < id || id >= maxId)
                break;

            minId = id + 1;
        }
    }

    ok = (minId <= maxId);

    return minId;
}
