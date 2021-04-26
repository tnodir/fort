#include "rpcmanager.h"

#include "../control/controlmanager.h"
#include "../control/controlworker.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../rpc/appinfomanagerrpc.h"
#include "../rpc/quotamanagerrpc.h"

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
    } else {
        // TODO: Initialize RPC Client
    }
}

void RpcManager::setupServerSignals()
{
    setupAppInfoManagerSignals();
    setupQuotaManagerSignals();
}

void RpcManager::setupAppInfoManagerSignals()
{
    constexpr qint8 rpcObj = Obj_AppInfoManager;
    auto o = fortManager()->appInfoManager();

    connect(o, &AppInfoManager::lookupFinished, this,
            [&](const QString &appPath, const AppInfo & /*appInfo*/) {
                invokeOnClients(rpcObj, "checkLookupFinished", { appPath });
            });
}

void RpcManager::setupQuotaManagerSignals()
{
    constexpr qint8 rpcObj = Obj_QuotaManager;
    auto o = fortManager()->quotaManager();

    connect(o, &QuotaManager::alert, this,
            [&](qint8 alertType) { invokeOnClients(rpcObj, "alert", { alertType }); });
}

void RpcManager::invokeOnServer(qint8 rpcObj, const char *member, const QVariantList &args)
{
    // TODO: Send RPC to Server
}

void RpcManager::invokeOnClients(qint8 rpcObj, const char *member, const QVariantList &args)
{
    const auto clients = controlManager()->clients();
    for (ControlWorker *w : clients) {
        if (!w->isServiceClient())
            continue;

        // TODO: Send RPC to Client
    }
}
