#include "statmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

StatManagerRpc::StatManagerRpc(const QString &filePath, FortManager *fortManager, QObject *parent) :
    StatManager(filePath, fortManager->quotaManager(), parent, SqliteDb::OpenDefaultReadOnly)
{
}

RpcManager *StatManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}
