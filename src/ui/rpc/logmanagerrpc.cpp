#include "logmanagerrpc.h"

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

LogManagerRpc::LogManagerRpc(FortManager *fortManager, QObject *parent) :
    LogManager(fortManager, parent)
{
}

RpcManager *LogManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void LogManagerRpc::setActive(bool /*active*/) { }

void LogManagerRpc::initialize() { }

void LogManagerRpc::close() { }
