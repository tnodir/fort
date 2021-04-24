#include "quotamanagerrpc.h"

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

QuotaManagerRpc::QuotaManagerRpc(FortManager *fortManager, QObject *parent) :
    QuotaManager(fortManager->settings(), parent), m_fortManager(fortManager)
{
}

RpcManager *QuotaManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void QuotaManagerRpc::initialize()
{
    // rpcManager()->connectSignal("quotaManager", "alert", this, &QuotaManager::alert);
}
