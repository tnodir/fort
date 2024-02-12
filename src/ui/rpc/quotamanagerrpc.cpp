#include "quotamanagerrpc.h"

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

QuotaManagerRpc::QuotaManagerRpc(QObject *parent) : QuotaManager(parent) { }

void QuotaManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto quotaManager = IoC<QuotaManager>();

    connect(quotaManager, &QuotaManager::alert, rpcManager, [&](qint8 alertType) {
        rpcManager->invokeOnClients(Control::Rpc_QuotaManager_alert, { alertType });
    });
}
