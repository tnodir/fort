#ifndef ACTIVITYMANAGERRPC_H
#define ACTIVITYMANAGERRPC_H

#include <activity/activitymanager.h>
#include <control/control_types.h>

class RpcManager;

class ActivityManagerRpc : public ActivityManager
{
    Q_OBJECT

public:
    explicit ActivityManagerRpc(const QString &filePath, QObject *parent = nullptr);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // ACTIVITYMANAGERRPC_H
