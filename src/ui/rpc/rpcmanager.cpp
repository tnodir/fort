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

AppInfoManager *RpcManager::appInfoManager() const
{
    return fortManager()->appInfoManager();
}

ConfManager *RpcManager::confManager() const
{
    return fortManager()->confManager();
}

DriverManager *RpcManager::driverManager() const
{
    return fortManager()->driverManager();
}

QuotaManager *RpcManager::quotaManager() const
{
    return fortManager()->quotaManager();
}

StatManager *RpcManager::statManager() const
{
    return fortManager()->statManager();
}

TaskManager *RpcManager::taskManager() const
{
    return fortManager()->taskManager();
}

void RpcManager::initialize()
{
    if (settings()->isService()) {
        setupServerSignals();
    }
}

void RpcManager::setupServerSignals()
{
    setupAppInfoManagerSignals();
    setupQuotaManagerSignals();
}

void RpcManager::setupAppInfoManagerSignals()
{
    connect(appInfoManager(), &AppInfoManager::lookupFinished, this,
            [&](const QString &appPath, const AppInfo & /*appInfo*/) {
                invokeOnClients(Control::Rpc_AppInfoManager_checkLookupFinished, { appPath });
            });
}

void RpcManager::setupQuotaManagerSignals()
{
    connect(quotaManager(), &QuotaManager::alert, this, [&](qint8 alertType) {
        invokeOnClients(Control::Rpc_QuotaManager_alert, { alertType });
    });
}

void RpcManager::invokeOnServer(Control::Command cmd, const QVariantList &args)
{
    m_client->sendCommand(cmd, args);
}

void RpcManager::invokeOnClients(Control::Command cmd, const QVariantList &args)
{
    const auto clients = controlManager()->clients();
    for (ControlWorker *w : clients) {
        if (!w->isServiceClient())
            continue;

        w->sendCommand(cmd, args);
    }
}

bool RpcManager::processCommandRpc(
        Control::Command cmd, const QVariantList &args, QString &errorMessage)
{
    switch (cmd) {
    case Control::Rpc_AppInfoManager_lookupAppInfo:
        appInfoManager()->lookupAppInfo(args.value(0).toString());
        return true;
    case Control::Rpc_AppInfoManager_checkLookupFinished:
        appInfoManager()->checkLookupFinished(args.value(0).toString());
        return true;
    case Control::Rpc_ConfManager_:
        confManager();
        return true;
    case Control::Rpc_DriverManager_:
        driverManager();
        return true;
    case Control::Rpc_QuotaManager_alert:
        emit quotaManager()->alert(args.value(0).toInt());
        return true;
    case Control::Rpc_StatManager_:
        statManager();
        return true;
    case Control::Rpc_TaskManager_:
        taskManager();
        return true;
    default:
        errorMessage = "Unknown command";
        return false;
    }
}
