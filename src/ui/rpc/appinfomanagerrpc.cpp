#include "appinfomanagerrpc.h"

#include <sqlite/sqlitedb.h>

AppInfoManagerRpc::AppInfoManagerRpc(const QString &filePath, QObject *parent) :
    AppInfoManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

void AppInfoManagerRpc::updateAppAccessTime(const QString & /*appPath*/) { }
