#include "taskmanagerrpc.h"

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

TaskManagerRpc::TaskManagerRpc(FortManager *fortManager, QObject *parent) :
    TaskManager(fortManager, parent)
{
}

RpcManager *TaskManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void TaskManagerRpc::initialize()
{
    loadSettings();
}
