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

    void onConfSaved(bool onlyFlags, int confVersion);

protected:
    void setupAppEndTimer() override;
};

#endif // CONFMANAGERRPC_H
