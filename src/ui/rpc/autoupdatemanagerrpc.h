#ifndef AUTOUPDATEMANAGERRPC_H
#define AUTOUPDATEMANAGERRPC_H

#include <manager/autoupdatemanager.h>

class RpcManager;

struct ProcessCommandArgs;

class AutoUpdateManagerRpc : public AutoUpdateManager
{
    Q_OBJECT

public:
    explicit AutoUpdateManagerRpc(const QString &cachePath, QObject *parent = nullptr);

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // AUTOUPDATEMANAGERRPC_H
