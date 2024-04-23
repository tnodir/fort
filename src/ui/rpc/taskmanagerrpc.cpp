#include "taskmanagerrpc.h"

#include <rpc/rpcmanager.h>
#include <task/taskinfo.h>
#include <util/ioc/ioccontainer.h>

namespace {

inline bool processTaskManager_runTask(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    taskManager->runTask(p.args.value(0).toInt());
    return true;
}

inline bool processTaskManager_abortTask(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    taskManager->abortTask(p.args.value(0).toInt());
    return true;
}

inline bool processTaskManager_taskStarted(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    if (auto tm = qobject_cast<TaskManagerRpc *>(taskManager)) {
        tm->onTaskStarted(p.args.value(0).toInt());
    }
    return true;
}

inline bool processTaskManager_taskFinished(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    if (auto tm = qobject_cast<TaskManagerRpc *>(taskManager)) {
        tm->onTaskFinished(p.args.value(0).toInt());
    }
    return true;
}

}

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

bool TaskManagerRpc::processServerCommand(const ProcessCommandArgs &p, QVariantList & /*resArgs*/,
        bool & /*ok*/, bool & /*isSendResult*/)
{
    auto taskManager = IoC<TaskManager>();

    switch (p.command) {
    case Control::Rpc_TaskManager_runTask: {
        return processTaskManager_runTask(taskManager, p);
    }
    case Control::Rpc_TaskManager_abortTask: {
        return processTaskManager_abortTask(taskManager, p);
    }
    case Control::Rpc_TaskManager_taskStarted: {
        return processTaskManager_taskStarted(taskManager, p);
    }
    case Control::Rpc_TaskManager_taskFinished: {
        return processTaskManager_taskFinished(taskManager, p);
    }
    case Control::Rpc_TaskManager_appVersionDownloaded: {
        emit taskManager->appVersionDownloaded(p.args[0].toString());
        return true;
    }
    case Control::Rpc_TaskManager_zonesDownloaded: {
        emit taskManager->zonesDownloaded(p.args[0].toStringList());
        return true;
    }
    default:
        return false;
    }
}

void TaskManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto taskManager = IoC<TaskManager>();

    connect(taskManager, &TaskManager::taskStarted, rpcManager, [=](qint8 taskType) {
        rpcManager->invokeOnClients(Control::Rpc_TaskManager_taskStarted, { taskType });
    });
    connect(taskManager, &TaskManager::taskFinished, rpcManager, [=](qint8 taskType) {
        rpcManager->invokeOnClients(Control::Rpc_TaskManager_taskFinished, { taskType });
    });

    connect(taskManager, &TaskManager::appVersionDownloaded, rpcManager,
            [=](const QString &version) {
                rpcManager->invokeOnClients(
                        Control::Rpc_TaskManager_appVersionDownloaded, { version });
            });
    connect(taskManager, &TaskManager::zonesDownloaded, rpcManager,
            [=](const QStringList &zoneNames) {
                rpcManager->invokeOnClients(
                        Control::Rpc_TaskManager_zonesDownloaded, { zoneNames });
            });
}
