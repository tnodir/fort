#include "sqliteutilbase.h"

#include "sqlitedb.h"

bool SqliteUtilBase::beginWriteTransaction()
{
    return sqliteDb()->beginWriteTransaction();
}

void SqliteUtilBase::commitTransaction()
{
    sqliteDb()->commitTransaction();
}

void SqliteUtilBase::endTransaction(bool &ok)
{
    sqliteDb()->endTransaction(ok);
}
