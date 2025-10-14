#include "appinfomanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

AppInfoManagerRpc::AppInfoManagerRpc(const QString &filePath, bool noCache, QObject *parent) :
    AppInfoManager(filePath, parent,
            (noCache ? SqliteDb::OpenDefaultReadWrite : SqliteDb::OpenDefaultReadOnly))
{
}

void AppInfoManagerRpc::lookupAppInfo(const QString &appPath)
{
    rpcManager()->invokeOnServer(Control::Rpc_AppInfoManager_lookupAppInfo, { appPath });
}

bool AppInfoManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    auto appInfoManager = Fort::appInfoManager();

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
    auto appInfoManager = Fort::appInfoManager();

    connect(appInfoManager, &AppInfoManager::lookupInfoFinished, rpcManager,
            [=](const QString &appPath, const AppInfo & /*appInfo*/) {
                rpcManager->invokeOnClients(
                        Control::Rpc_AppInfoManager_checkLookupInfoFinished, { appPath });
            });
}
