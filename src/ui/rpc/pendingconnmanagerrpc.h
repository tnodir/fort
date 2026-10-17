#ifndef PENDINGCONNMANAGERRPC_H
#define PENDINGCONNMANAGERRPC_H

#include <control/control_types.h>
#include <stat/pendingconnmanager.h>

class RpcManager;

class PendingConnManagerRpc : public PendingConnManager
{
    Q_OBJECT

public:
    explicit PendingConnManagerRpc(const QString &filePath, QObject *parent = nullptr);

    bool decideConn(qint64 connId, ConnDecision decision) override;

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

protected:
    void setupWorker() override { }

    void checkClearConnOnStartup() override { }
    void checkClearConnOnExit() override { }
};

#endif // PENDINGCONNMANAGERRPC_H
