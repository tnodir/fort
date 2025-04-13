#ifndef TASKMANAGERRPC_H
#define TASKMANAGERRPC_H

#include <control/control_types.h>
#include <task/taskmanager.h>

class RpcManager;

class TaskManagerRpc : public TaskManager
{
    Q_OBJECT

public:
    explicit TaskManagerRpc(QObject *parent = nullptr);

    void onTaskStarted(qint8 taskType);
    void onTaskFinished(qint8 taskType);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    void runTask(qint8 taskType) override;
    void abortTask(qint8 taskType) override;

protected slots:
    void runExpiredTasks() override { }

protected:
    void initializeTasks() override { }

    void setupTimer(int /*secs*/) override { }
};

#endif // TASKMANAGERRPC_H
