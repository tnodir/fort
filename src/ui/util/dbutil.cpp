#include "dbutil.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

int DbUtil::getFreeId(SqliteDb *sqliteDb, const char *sqlSelectIds, int maxCount, bool &ok)
{
    int freeId = 1;

    SqliteStmt stmt;
    if (stmt.prepare(sqliteDb->db(), sqlSelectIds)) {
        while (stmt.step() == SqliteStmt::StepRow) {
            const int id = stmt.columnInt(0);
            if (freeId < id)
                break;

            freeId = id + 1;
        }
    }

    ok = (freeId < maxCount);

    return freeId;
}
