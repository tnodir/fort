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

    void initialize() override;
};

#endif // TASKMANAGERRPC_H
