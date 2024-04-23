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

bool AppInfoManagerRpc::processServerCommand(const ProcessCommandArgs &p,
        QVariantList & /*resArgs*/, bool & /*ok*/, bool & /*isSendResult*/)
{
    auto appInfoManager = IoC<AppInfoManager>();

    switch (p.command) {
    case Control::Rpc_AppInfoManager_lookupAppInfo: {
        appInfoManager->lookupAppInfo(p.args.value(0).toString());
        return true;
    }
    case Control::Rpc_AppInfoManager_checkLookupInfoFinished: {
        appInfoManager->checkLookupInfoFinished(p.args.value(0).toString());
        return true;
    }
    default:
        return false;
    }
}

void AppInfoManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto appInfoManager = IoC<AppInfoManager>();

    connect(appInfoManager, &AppInfoManager::lookupInfoFinished, rpcManager,
            [=](const QString &appPath, const AppInfo & /*appInfo*/) {
                rpcManager->invokeOnClients(
                        Control::Rpc_AppInfoManager_checkLookupInfoFinished, { appPath });
            });
}
