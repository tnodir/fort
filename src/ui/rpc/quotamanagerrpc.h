#ifndef QUOTAMANAGERRPC_H
#define QUOTAMANAGERRPC_H

#include <control/control_types.h>
#include <stat/quotamanager.h>

class RpcManager;

class QuotaManagerRpc : public QuotaManager
{
    Q_OBJECT

public:
    explicit QuotaManagerRpc(QObject *parent = nullptr);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

protected:
    void setupConfManager() override { }
};

#endif // QUOTAMANAGERRPC_H
