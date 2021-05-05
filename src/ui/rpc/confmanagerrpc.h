#ifndef CONFMANAGERRPC_H
#define CONFMANAGERRPC_H

#include "../conf/confmanager.h"

class RpcManager;

class ConfManagerRpc : public ConfManager
{
    Q_OBJECT

public:
    explicit ConfManagerRpc(
            const QString &filePath, FortManager *fortManager, QObject *parent = nullptr);

    RpcManager *rpcManager() const;

    void onConfChanged(int confVersion, bool onlyFlags);

protected:
    void setupAppEndTimer() override { }

    bool saveConf(FirewallConf &newConf, bool onlyFlags) override;
};

#endif // CONFMANAGERRPC_H
