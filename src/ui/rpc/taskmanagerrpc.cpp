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

void TaskManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto taskManager = IoC<TaskManager>();

    connect(taskManager, &TaskManager::taskStarted, rpcManager, [&](qint8 taskType) {
        rpcManager->invokeOnClients(Control::Rpc_TaskManager_taskStarted, { taskType });
    });
    connect(taskManager, &TaskManager::taskFinished, rpcManager, [&](qint8 taskType) {
        rpcManager->invokeOnClients(Control::Rpc_TaskManager_taskFinished, { taskType });
    });

    connect(taskManager, &TaskManager::appVersionDownloaded, rpcManager,
            [&](const QString &version) {
                rpcManager->invokeOnClients(
                        Control::Rpc_TaskManager_appVersionDownloaded, { version });
            });
    connect(taskManager, &TaskManager::zonesDownloaded, rpcManager,
            [&](const QStringList &zoneNames) {
                rpcManager->invokeOnClients(
                        Control::Rpc_TaskManager_zonesDownloaded, { zoneNames });
            });
}
