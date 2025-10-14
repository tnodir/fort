#include "autoupdatemanagerrpc.h"

#include <control/controlworker.h>
#include <fortglobal.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

using namespace Fort;

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
        AutoUpdateManager *autoUpdateManager, const ProcessCommandArgs &p)
{
    const bool restarting = p.args.value(0).toBool();

    if (qobject_cast<AutoUpdateManagerRpc *>(autoUpdateManager)) {
        autoUpdateManager->onRestartClient(restarting);
    } else {
        emit autoUpdateManager->restartClients(restarting);
    }
    return true;
}

bool processAutoUpdateManager_startDownload(AutoUpdateManager *autoUpdateManager,
        const ProcessCommandArgs & /*p*/, ProcessCommandResult & /*r*/)
{
    return autoUpdateManager->startDownload();
}

bool processAutoUpdateManager_runInstaller(AutoUpdateManager *autoUpdateManager,
        const ProcessCommandArgs & /*p*/, ProcessCommandResult & /*r*/)
{
    return autoUpdateManager->runInstaller();
}

using processAutoUpdateManager_func = bool (*)(
        AutoUpdateManager *autoUpdateManager, const ProcessCommandArgs &p, ProcessCommandResult &r);

static const processAutoUpdateManager_func processAutoUpdateManager_funcList[] = {
    &processAutoUpdateManager_startDownload, // Rpc_AutoUpdateManager_startDownload,
    &processAutoUpdateManager_runInstaller, // Rpc_AutoUpdateManager_runInstaller,
};

inline bool processAutoUpdateManagerRpcResult(
        AutoUpdateManager *autoUpdateManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processAutoUpdateManager_func func = RpcManager::getProcessFunc(p.command,
            processAutoUpdateManager_funcList, Control::Rpc_AutoUpdateManager_startDownload,
            Control::Rpc_AutoUpdateManager_runInstaller);

    return func ? func(autoUpdateManager, p, r) : false;
}

}

AutoUpdateManagerRpc::AutoUpdateManagerRpc(const QString &updatePath, QObject *parent) :
    AutoUpdateManager(updatePath, parent)
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
    auto autoUpdateManager = Fort::autoUpdateManager();

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
        const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto autoUpdateManager = Fort::autoUpdateManager();

    switch (p.command) {
    case Control::Rpc_AutoUpdateManager_updateState: {
        return processAutoUpdateManager_updateState(autoUpdateManager, p);
    }
    case Control::Rpc_AutoUpdateManager_restartClients: {
        return processAutoUpdateManager_restartClients(autoUpdateManager, p);
    }
    default: {
        r.ok = processAutoUpdateManagerRpcResult(autoUpdateManager, p, r);
        r.isSendResult = true;
        return true;
    }
    }
}

void AutoUpdateManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto autoUpdateManager = Fort::autoUpdateManager();

    const auto updateClientStates = [=] {
        rpcManager->invokeOnClients(Control::Rpc_AutoUpdateManager_updateState,
                AutoUpdateManagerRpc::updateState_args());
    };

    connect(autoUpdateManager, &AutoUpdateManager::flagsChanged, rpcManager, updateClientStates);
    connect(autoUpdateManager, &AutoUpdateManager::bytesReceivedChanged, rpcManager,
            updateClientStates);

    connect(autoUpdateManager, &AutoUpdateManager::restartClients, rpcManager,
            [=](bool restarting) {
                rpcManager->invokeOnClients(
                        Control::Rpc_AutoUpdateManager_restartClients, { restarting });
            });
}

bool AutoUpdateManagerRpc::startDownload()
{
    return rpcManager()->doOnServer(Control::Rpc_AutoUpdateManager_startDownload);
}

bool AutoUpdateManagerRpc::runInstaller()
{
    return rpcManager()->doOnServer(Control::Rpc_AutoUpdateManager_runInstaller);
}

void AutoUpdateManagerRpc::setupManager()
{
    setupClientSignals();
}

void AutoUpdateManagerRpc::setupClientSignals()
{
    connect(this, &AutoUpdateManager::restartClients, rpcManager(), [=](bool restarting) {
        rpcManager()->invokeOnServer(Control::Rpc_AutoUpdateManager_restartClients, { restarting });
    });
}
