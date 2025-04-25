#include "sqliteutilbase.h"

#include <util/fileutil.h>

#include "sqlitedb.h"

void SqliteUtilBase::removeDbFilesToCleanOpen() const
{
    const auto filePath = sqliteDb()->filePath();

    if (!FileUtil::fileExists(filePath + "-wal"))
        return;

    FileUtil::removeFilesByPrefix(filePath);
}

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
