#include "quotamanagerrpc.h"

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

QuotaManagerRpc::QuotaManagerRpc(QObject *parent) : QuotaManager(parent) { }

bool QuotaManagerRpc::processServerCommand(const ProcessCommandArgs &p, QVariantList & /*resArgs*/,
        bool & /*ok*/, bool & /*isSendResult*/)
{
    auto quotaManager = IoC<QuotaManager>();

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
    auto quotaManager = IoC<QuotaManager>();

    connect(quotaManager, &QuotaManager::alert, rpcManager, [=](qint8 alertType) {
        rpcManager->invokeOnClients(Control::Rpc_QuotaManager_alert, { alertType });
    });
}
