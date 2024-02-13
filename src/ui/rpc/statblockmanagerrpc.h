#ifndef STATBLOCKMANAGERRPC_H
#define STATBLOCKMANAGERRPC_H

#include <stat/statblockmanager.h>

class RpcManager;

struct ProcessCommandArgs;

class StatBlockManagerRpc : public StatBlockManager
{
    Q_OBJECT

public:
    explicit StatBlockManagerRpc(const QString &filePath, QObject *parent = nullptr);

    void deleteConn(qint64 connIdTo = 0) override;

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);

protected:
    void setupWorker() override { }
    void setupConfManager() override { }
};

#endif // STATBLOCKMANAGERRPC_H
