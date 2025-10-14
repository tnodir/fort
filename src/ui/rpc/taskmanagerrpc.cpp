#include "taskmanagerrpc.h"

#include <fortglobal.h>
#include <rpc/rpcmanager.h>
#include <task/taskinfo.h>

using namespace Fort;

namespace {

bool processTaskManager_appVersionUpdated(
        TaskManager *taskManager, const ProcessCommandArgs & /*p*/)
{
    emit taskManager->appVersionUpdated();
    return true;
}

bool processTaskManager_appVersionDownloaded(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    emit taskManager->appVersionDownloaded(p.args[0].toString());
    return true;
}

bool processTaskManager_zonesDownloaded(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    emit taskManager->zonesDownloaded(p.args[0].toStringList());
    return true;
}

using processTaskManagerSignal_func = bool (*)(
        TaskManager *taskManager, const ProcessCommandArgs &p);

static const processTaskManagerSignal_func processTaskManagerSignal_funcList[] = {
    &processTaskManager_appVersionUpdated, // Rpc_TaskManager_appVersionUpdated,
    &processTaskManager_appVersionDownloaded, // Rpc_TaskManager_appVersionDownloaded,
    &processTaskManager_zonesDownloaded, // Rpc_TaskManager_zonesDownloaded,
};

inline bool processTaskManagerRpcSignal(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    const processTaskManagerSignal_func func = RpcManager::getProcessFunc(p.command,
            processTaskManagerSignal_funcList, Control::Rpc_TaskManager_appVersionUpdated,
            Control::Rpc_TaskManager_zonesDownloaded);

    return func ? func(taskManager, p) : false;
}

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

inline bool processTaskManagerRpcResult(TaskManager *taskManager, const ProcessCommandArgs &p)
{
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
    default:
        return false;
    }
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
    rpcManager()->invokeOnServer(Control::Rpc_TaskManager_runTask, { taskType });
}

void TaskManagerRpc::abortTask(qint8 taskType)
{
    rpcManager()->invokeOnServer(Control::Rpc_TaskManager_abortTask, { taskType });
}

bool TaskManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    auto taskManager = Fort::taskManager();

    switch (p.command) {
    case Control::Rpc_TaskManager_appVersionUpdated:
    case Control::Rpc_TaskManager_appVersionDownloaded:
    case Control::Rpc_TaskManager_zonesDownloaded: {
        return processTaskManagerRpcSignal(taskManager, p);
    }
    default:
        return processTaskManagerRpcResult(taskManager, p);
    }
}

void TaskManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto taskManager = Fort::taskManager();

    connect(taskManager, &TaskManager::taskStarted, rpcManager, [=](qint8 taskType) {
        rpcManager->invokeOnClients(Control::Rpc_TaskManager_taskStarted, { taskType });
    });
    connect(taskManager, &TaskManager::taskFinished, rpcManager, [=](qint8 taskType) {
        rpcManager->invokeOnClients(Control::Rpc_TaskManager_taskFinished, { taskType });
    });

    connect(taskManager, &TaskManager::appVersionUpdated, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_TaskManager_appVersionUpdated); });
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
