#include "autoupdatemanagerrpc.h"

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

AutoUpdateManagerRpc::AutoUpdateManagerRpc(const QString &cachePath, QObject *parent) :
    AutoUpdateManager(cachePath, parent)
{
}

bool AutoUpdateManagerRpc::processServerCommand(const ProcessCommandArgs &p,
        QVariantList & /*resArgs*/, bool & /*ok*/, bool & /*isSendResult*/)
{
    auto autoUpdateManager = IoC<AutoUpdateManager>();

    switch (p.command) {
    case Control::Rpc_AutoUpdateManager_restartClients:
        QMetaObject::invokeMethod(
                autoUpdateManager, [] { OsUtil::restartClient(); }, Qt::QueuedConnection);
        return true;
    default:
        return false;
    }
}

void AutoUpdateManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto autoUpdateManager = IoC<AutoUpdateManager>();

    connect(autoUpdateManager, &AutoUpdateManager::restartClients, rpcManager,
            [&] { rpcManager->invokeOnClients(Control::Rpc_AutoUpdateManager_restartClients); });
}
