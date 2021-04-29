#include "rpcmanager.h"

#include "../control/controlmanager.h"
#include "../control/controlworker.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../rpc/appinfomanagerrpc.h"
#include "../rpc/confmanagerrpc.h"
#include "../rpc/drivermanagerrpc.h"
#include "../rpc/quotamanagerrpc.h"
#include "../rpc/statmanagerrpc.h"
#include "../rpc/taskmanagerrpc.h"
#include "../util/classutil.h"

RpcManager::RpcManager(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
}

FortSettings *RpcManager::settings() const
{
    return fortManager()->settings();
}

ControlManager *RpcManager::controlManager() const
{
    return fortManager()->controlManager();
}

void RpcManager::initialize()
{
    if (settings()->isService()) {
        setupServerSignals();
    }
}

bool RpcManager::processCommandRpc(
        Control::RpcObject rpcObj, int methodIndex, const QVariantList &args, QString &errorMessage)
{
    QObject *o = getRpcObject(rpcObj);
    if (!o) {
        errorMessage = "Bad RPC: No object";
        return false;
    }

    return ClassUtil::invokeMethod(o, methodIndex, args);
}

void RpcManager::setupServerSignals()
{
    setupAppInfoManagerSignals();
    setupQuotaManagerSignals();
}

void RpcManager::setupAppInfoManagerSignals()
{
    constexpr Control::RpcObject rpcObj = Control::Rpc_AppInfoManager;
    auto o = fortManager()->appInfoManager();

    connect(o, &AppInfoManager::lookupFinished, this,
            [&](const QString &appPath, const AppInfo & /*appInfo*/) {
                static const int methodIndex =
                        ClassUtil::indexOfMethod(&AppInfoManager::checkLookupFinished);
                invokeOnClients(rpcObj, methodIndex, { appPath });
            });
}

void RpcManager::setupQuotaManagerSignals()
{
    constexpr Control::RpcObject rpcObj = Control::Rpc_QuotaManager;
    auto o = fortManager()->quotaManager();

    connect(o, &QuotaManager::alert, this, [&](qint8 alertType) {
        static const int methodIndex = ClassUtil::indexOfSignal(&QuotaManager::alert);
        invokeOnClients(rpcObj, methodIndex, { alertType });
    });
}

void RpcManager::invokeOnServer(
        Control::RpcObject rpcObj, int methodIndex, const QVariantList &args)
{
    m_client->sendCommand(Control::CommandRpc, rpcObj, methodIndex, args);
}

void RpcManager::invokeOnClients(
        Control::RpcObject rpcObj, int methodIndex, const QVariantList &args)
{
    const auto clients = controlManager()->clients();
    for (ControlWorker *w : clients) {
        if (!w->isServiceClient())
            continue;

        w->sendCommand(Control::CommandRpc, rpcObj, methodIndex, args);
    }
}

QObject *RpcManager::getRpcObject(Control::RpcObject rpcObj) const
{
    switch (rpcObj) {
    case Control::Rpc_AppInfoManager:
        return fortManager()->appInfoManager();
    case Control::Rpc_ConfManager:
        return fortManager()->confManager();
    case Control::Rpc_DriverManager:
        return fortManager()->driverManager();
    case Control::Rpc_QuotaManager:
        return fortManager()->quotaManager();
    case Control::Rpc_StatManager:
        return fortManager()->statManager();
    case Control::Rpc_TaskManager:
        return fortManager()->taskManager();
    default:
        return nullptr;
    }
}
