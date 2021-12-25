#include "taskmanagerrpc.h"

#include <rpc/rpcmanager.h>
#include <task/taskinfo.h>
#include <util/ioc/ioccontainer.h>

TaskManagerRpc::TaskManagerRpc(QObject *parent) : TaskManager(parent) { }

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
    IoC<RpcManager>()->invokeOnServer(Control::Rpc_TaskManager_runTask, { taskType });
}

void TaskManagerRpc::abortTask(qint8 taskType)
{
    IoC<RpcManager>()->invokeOnServer(Control::Rpc_TaskManager_abortTask, { taskType });
}
