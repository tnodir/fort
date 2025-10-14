#include "quotamanagerrpc.h"

#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

QuotaManagerRpc::QuotaManagerRpc(QObject *parent) : QuotaManager(parent) { }

bool QuotaManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    auto quotaManager = Fort::quotaManager();

    switch (p.command) {
    case Control::Rpc_QuotaManager_alert: {
        emit quotaManager->alert(p.args.value(0).toInt());
        return true;
    }
    default:
        return false;
    }
}

void QuotaManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto quotaManager = Fort::quotaManager();

    connect(quotaManager, &QuotaManager::alert, rpcManager, [=](qint8 alertType) {
        rpcManager->invokeOnClients(Control::Rpc_QuotaManager_alert, { alertType });
    });
}
