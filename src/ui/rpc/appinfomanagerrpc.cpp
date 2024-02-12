#include "appinfomanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

AppInfoManagerRpc::AppInfoManagerRpc(const QString &filePath, QObject *parent) :
    AppInfoManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

void AppInfoManagerRpc::lookupAppInfo(const QString &appPath)
{
    IoC<RpcManager>()->invokeOnServer(Control::Rpc_AppInfoManager_lookupAppInfo, { appPath });
}

void AppInfoManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto appInfoManager = IoC<AppInfoManager>();

    connect(appInfoManager, &AppInfoManager::lookupInfoFinished, rpcManager,
            [&](const QString &appPath, const AppInfo & /*appInfo*/) {
                rpcManager->invokeOnClients(
                        Control::Rpc_AppInfoManager_checkLookupInfoFinished, { appPath });
            });
}
