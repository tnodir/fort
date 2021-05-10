#ifndef TASKMANAGERRPC_H
#define TASKMANAGERRPC_H

#include "../task/taskmanager.h"

class RpcManager;

class TaskManagerRpc : public TaskManager
{
    Q_OBJECT

public:
    explicit TaskManagerRpc(FortManager *fortManager, QObject *parent = nullptr);

    RpcManager *rpcManager() const;

    void onTaskStarted(qint8 taskType);
    void onTaskFinished(qint8 taskType);

public slots:
    void runTask(qint8 taskType) override;
    void abortTask(qint8 taskType) override;

protected:
    void setupScheduler() override { }
};

#endif // TASKMANAGERRPC_H
