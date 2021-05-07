#include "quotamanagerrpc.h"

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

QuotaManagerRpc::QuotaManagerRpc(FortManager *fortManager, QObject *parent) :
    QuotaManager(fortManager->confManager(), parent), m_fortManager(fortManager)
{
}

RpcManager *QuotaManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}
