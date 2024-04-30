#include "autoupdatemanagerrpc.h"

#include <control/controlworker.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

namespace {

inline bool processAutoUpdateManager_updateState(
        AutoUpdateManager *autoUpdateManager, const ProcessCommandArgs &p)
{
    if (auto aum = qobject_cast<AutoUpdateManagerRpc *>(autoUpdateManager)) {
        aum->updateState(AutoUpdateManager::Flags(p.args.value(0).toInt()), p.args.value(1).toInt(),
                p.args.value(2).toString());
    }
    return true;
}

inline bool processAutoUpdateManager_restartClients(
        AutoUpdateManager *autoUpdateManager, const ProcessCommandArgs & /*p*/)
{
    if (qobject_cast<AutoUpdateManagerRpc *>(autoUpdateManager)) {
        OsUtil::restartClient();
    } else {
        emit autoUpdateManager->restartClients();
    }
    return true;
}

bool processAutoUpdateManager_startDownload(AutoUpdateManager *autoUpdateManager,
        const ProcessCommandArgs & /*p*/, QVariantList & /*resArgs*/)
{
    return autoUpdateManager->startDownload();
}

bool processAutoUpdateManager_runInstaller(AutoUpdateManager *autoUpdateManager,
        const ProcessCommandArgs & /*p*/, QVariantList & /*resArgs*/)
{
    return autoUpdateManager->runInstaller();
}

using processAutoUpdateManager_func = bool (*)(
        AutoUpdateManager *autoUpdateManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processAutoUpdateManager_func processAutoUpdateManager_funcList[] = {
    &processAutoUpdateManager_startDownload, // Rpc_AutoUpdateManager_startDownload,
    &processAutoUpdateManager_runInstaller, // Rpc_AutoUpdateManager_runInstaller,
};

inline bool processAutoUpdateManagerRpcResult(
        AutoUpdateManager *autoUpdateManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const processAutoUpdateManager_func func = RpcManager::getProcessFunc(p.command,
            processAutoUpdateManager_funcList, Control::Rpc_AutoUpdateManager_startDownload,
            Control::Rpc_AutoUpdateManager_runInstaller);

    return func ? func(autoUpdateManager, p, resArgs) : false;
}

}

AutoUpdateManagerRpc::AutoUpdateManagerRpc(const QString &cachePath, QObject *parent) :
    AutoUpdateManager(cachePath, parent)
{
}

void AutoUpdateManagerRpc::setBytesReceived(int v)
{
    if (m_bytesReceived != v) {
        m_bytesReceived = v;
        emit bytesReceivedChanged(v);
    }
}

void AutoUpdateManagerRpc::updateState(
        AutoUpdateManager::Flags flags, int bytesReceived, const QString &fileName)
{
    setFlags(flags);
    setBytesReceived(bytesReceived);
    setFileName(fileName);
}

QVariantList AutoUpdateManagerRpc::updateState_args()
{
    auto autoUpdateManager = IoC<AutoUpdateManager>();

    const auto flags = autoUpdateManager->flags();
    const auto bytesReceived = autoUpdateManager->bytesReceived();
    const auto fileName = autoUpdateManager->fileName();

    return { int(flags), bytesReceived, fileName };
}

bool AutoUpdateManagerRpc::processInitClient(ControlWorker *w)
{
    return w->sendCommand(Control::Rpc_AutoUpdateManager_updateState, updateState_args());
}

bool AutoUpdateManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult)
{
    auto autoUpdateManager = IoC<AutoUpdateManager>();

    switch (p.command) {
    case Control::Rpc_AutoUpdateManager_updateState: {
        return processAutoUpdateManager_updateState(autoUpdateManager, p);
    }
    case Control::Rpc_AutoUpdateManager_restartClients: {
        return processAutoUpdateManager_restartClients(autoUpdateManager, p);
    }
    default: {
        ok = processAutoUpdateManagerRpcResult(autoUpdateManager, p, resArgs);
        isSendResult = true;
        return true;
    }
    }
}

void AutoUpdateManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto autoUpdateManager = IoC<AutoUpdateManager>();

    const auto updateClientStates = [=] {
        rpcManager->invokeOnClients(Control::Rpc_AutoUpdateManager_updateState,
                AutoUpdateManagerRpc::updateState_args());
    };

    connect(autoUpdateManager, &AutoUpdateManager::flagsChanged, rpcManager, updateClientStates);
    connect(autoUpdateManager, &AutoUpdateManager::bytesReceivedChanged, rpcManager,
            updateClientStates);

    connect(autoUpdateManager, &AutoUpdateManager::restartClients, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_AutoUpdateManager_restartClients); });
}

bool AutoUpdateManagerRpc::startDownload()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_AutoUpdateManager_startDownload);
}

bool AutoUpdateManagerRpc::runInstaller()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_AutoUpdateManager_runInstaller);
}

void AutoUpdateManagerRpc::setupManager()
{
    setupClientSignals();
}

void AutoUpdateManagerRpc::setupClientSignals()
{
    auto rpcManager = IoCDependency<RpcManager>();

    connect(this, &AutoUpdateManager::restartClients, rpcManager,
            [=] { rpcManager->invokeOnServer(Control::Rpc_AutoUpdateManager_restartClients); });
}
