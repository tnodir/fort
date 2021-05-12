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

bool StatManagerRpc::clear()
{
    if (!rpcManager()->doOnServer(Control::Rpc_StatManager_clear))
        return false;

    // TODO: re-open the DB

    return true;
}
