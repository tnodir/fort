#include "statblockmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

StatBlockManagerRpc::StatBlockManagerRpc(const QString &filePath, QObject *parent) :
    StatBlockManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool StatBlockManagerRpc::deleteConn(qint64 rowIdTo)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatBlockManager_deleteConn, { rowIdTo });
}

bool StatBlockManagerRpc::deleteConnAll()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatBlockManager_deleteConnAll);
}

void StatBlockManagerRpc::onConnChanged()
{
    setIsConnIdRangeUpdated(false);

    emit connChanged();
}
