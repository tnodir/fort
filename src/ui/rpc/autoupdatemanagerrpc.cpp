#include "autoupdatemanagerrpc.h"

#include <control/controlworker.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

AutoUpdateManagerRpc::AutoUpdateManagerRpc(const QString &cachePath, QObject *parent) :
    AutoUpdateManager(cachePath, parent)
{
}

void AutoUpdateManagerRpc::setIsDownloading(bool v)
{
    if (m_isDownloading != v) {
        m_isDownloading = v;
        emit isDownloadingChanged();
    }
}

void AutoUpdateManagerRpc::setBytesReceived(int v)
{
    if (m_bytesReceived != v) {
        m_bytesReceived = v;
        emit bytesReceivedChanged();
    }
}

void AutoUpdateManagerRpc::setUp()
{
    AutoUpdateManager::setUp();

    setupClientSignals();
}

void AutoUpdateManagerRpc::updateState(bool isDownloading, int bytesReceived)
{
    setIsDownloading(isDownloading);
    setBytesReceived(bytesReceived);
}

QVariantList AutoUpdateManagerRpc::updateState_args()
{
    auto autoUpdateManager = IoC<AutoUpdateManager>();

    return { autoUpdateManager->isDownloading(), autoUpdateManager->bytesReceived() };
}

bool AutoUpdateManagerRpc::processInitClient(ControlWorker *w)
{
    return w->sendCommand(Control::Rpc_AutoUpdateManager_updateState, updateState_args());
}

bool AutoUpdateManagerRpc::processServerCommand(const ProcessCommandArgs &p,
        QVariantList & /*resArgs*/, bool & /*ok*/, bool & /*isSendResult*/)
{
    auto autoUpdateManager = IoC<AutoUpdateManager>();

    switch (p.command) {
    case Control::Rpc_AutoUpdateManager_updateState: {
        if (auto aum = qobject_cast<AutoUpdateManagerRpc *>(autoUpdateManager)) {
            aum->updateState(p.args.value(0).toBool(), p.args.value(1).toInt());
        }
        return true;
    }
    case Control::Rpc_AutoUpdateManager_restartClients: {
        QMetaObject::invokeMethod(
                autoUpdateManager, [] { OsUtil::restartClient(); }, Qt::QueuedConnection);
        return true;
    }
    default:
        return false;
    }
}

void AutoUpdateManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto autoUpdateManager = IoC<AutoUpdateManager>();

    const auto updateClientStates = [=] {
        rpcManager->invokeOnClients(Control::Rpc_AutoUpdateManager_updateState,
                AutoUpdateManagerRpc::updateState_args());
    };

    connect(autoUpdateManager, &AutoUpdateManager::isDownloadingChanged, rpcManager,
            updateClientStates);
    connect(autoUpdateManager, &AutoUpdateManager::bytesReceivedChanged, rpcManager,
            updateClientStates);

    connect(autoUpdateManager, &AutoUpdateManager::restartClients, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_AutoUpdateManager_restartClients); });
}

void AutoUpdateManagerRpc::setupClientSignals()
{
    auto rpcManager = IoCDependency<RpcManager>();

    connect(this, &AutoUpdateManager::restartClients, rpcManager,
            [=] { rpcManager->invokeOnServer(Control::Rpc_AutoUpdateManager_restartClients); });
}
