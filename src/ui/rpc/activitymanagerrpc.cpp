#include "activitymanagerrpc.h"

#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

ActivityManagerRpc::ActivityManagerRpc(const QString &filePath, QObject *parent) :
    ActivityManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool ActivityManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    Q_UNUSED(r);

    switch (p.command) {
    case Control::Rpc_ActivityManager_changed: {
        emit Fort::activityManager()->activityChanged();
        return true;
    }
    default:
        return false;
    }
}

void ActivityManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    connect(activityManager(), &ActivityManager::activityChanged, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ActivityManager_changed); });
}
