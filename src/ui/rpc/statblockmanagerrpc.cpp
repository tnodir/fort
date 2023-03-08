#include "statblockmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

StatBlockManagerRpc::StatBlockManagerRpc(const QString &filePath, QObject *parent) :
    StatBlockManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

void StatBlockManagerRpc::deleteConn(qint64 connIdTo, int keepCount)
{
    IoC<RpcManager>()->doOnServer(
            Control::Rpc_StatBlockManager_deleteConn, { connIdTo, keepCount });
}
