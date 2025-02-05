#ifndef STATCONNMANAGERRPC_H
#define STATCONNMANAGERRPC_H

#include <stat/statconnmanager.h>

class RpcManager;

struct ProcessCommandArgs;

class StatConnManagerRpc : public StatConnManager
{
    Q_OBJECT

public:
    explicit StatConnManagerRpc(const QString &filePath, QObject *parent = nullptr);

    void deleteConn(qint64 connIdTo = 0) override;

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);

protected:
    void setupWorker() override { }
    void setupConfManager() override { }
};

#endif // STATCONNMANAGERRPC_H
