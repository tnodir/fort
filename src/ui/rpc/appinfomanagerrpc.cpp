#include "appinfomanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"
#include "../util/metaclassutil.h"

AppInfoManagerRpc::AppInfoManagerRpc(
        const QString &filePath, FortManager *fortManager, QObject *parent) :
    AppInfoManager(filePath, parent, SqliteDb::OpenDefaultReadOnly), m_fortManager(fortManager)
{
}

RpcManager *AppInfoManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void AppInfoManagerRpc::lookupAppInfo(const QString &appPath)
{
    static const int methodIndex = MetaClassUtil::indexOfMethod(&AppInfoManager::lookupAppInfo);
    rpcManager()->invokeOnServer(Control::Rpc_AppInfoManager, methodIndex, { appPath });
}

void AppInfoManagerRpc::updateAppAccessTime(const QString & /*appPath*/) { }
