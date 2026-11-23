#include "optimizeconndbjob.h"

#include <sqlite/sqlitedb.h>

void OptimizeConnDbJob::processJob()
{
    sqliteDb()->optimize();
}
