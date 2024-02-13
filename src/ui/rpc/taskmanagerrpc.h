#ifndef TASKMANAGERRPC_H
#define TASKMANAGERRPC_H

#include <task/taskmanager.h>

class RpcManager;

struct ProcessCommandArgs;

class TaskManagerRpc : public TaskManager
{
    Q_OBJECT

public:
    explicit TaskManagerRpc(QObject *parent = nullptr);

    void onTaskStarted(qint8 taskType);
    void onTaskFinished(qint8 taskType);

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    void runTask(qint8 taskType) override;
    void abortTask(qint8 taskType) override;

protected:
    void setupTimer(bool /*enabled*/ = true) override { }
};

#endif // TASKMANAGERRPC_H
