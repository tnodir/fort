#include "taskmanagerrpc.h"

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"
#include "../task/taskinfo.h"

TaskManagerRpc::TaskManagerRpc(FortManager *fortManager, QObject *parent) :
    TaskManager(fortManager, parent)
{
}

RpcManager *TaskManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void TaskManagerRpc::onTaskStarted(qint8 taskType)
{
    auto taskInfo = taskInfoByType(taskType);
    if (Q_UNLIKELY(!taskInfo))
        return;

    taskInfo->setRunning(true);

    emit taskStarted(taskType);
}

void TaskManagerRpc::onTaskFinished(qint8 taskType)
{
    auto taskInfo = taskInfoByType(taskType);
    if (Q_UNLIKELY(!taskInfo))
        return;

    taskInfo->setRunning(false);

    loadSettings();

    emit taskFinished(taskType);
}

void TaskManagerRpc::runTask(qint8 taskType)
{
    rpcManager()->invokeOnServer(Control::Rpc_TaskManager_runTask, { taskType });
}

void TaskManagerRpc::abortTask(qint8 taskType)
{
    rpcManager()->invokeOnServer(Control::Rpc_TaskManager_abortTask, { taskType });
}
