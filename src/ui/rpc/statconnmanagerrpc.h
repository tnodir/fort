#ifndef STATCONNMANAGERRPC_H
#define STATCONNMANAGERRPC_H

#include <control/control_types.h>
#include <stat/statconnmanager.h>

class RpcManager;

class StatConnManagerRpc : public StatConnManager
{
    Q_OBJECT

public:
    explicit StatConnManagerRpc(const QString &filePath, QObject *parent = nullptr);

    void deleteConn(qint64 connIdTo = 0) override;

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

protected:
    void setupWorker() override { }
    void setupConfManager() override { }

    void checkCearConnOnStartup() override { }
    void checkCearConnOnExit() override { }
};

#endif // STATCONNMANAGERRPC_H
