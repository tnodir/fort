#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

ConfManagerRpc::ConfManagerRpc(const QString &filePath, FortManager *fortManager, QObject *parent) :
    ConfManager(filePath, fortManager, parent, SqliteDb::OpenDefaultReadOnly)
{
}

RpcManager *ConfManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void ConfManagerRpc::setupAppEndTimer() { }
