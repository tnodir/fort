#include "statblockmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

StatBlockManagerRpc::StatBlockManagerRpc(const QString &filePath, QObject *parent) :
    StatBlockManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

void StatBlockManagerRpc::deleteConn(qint64 connIdTo)
{
    IoC<RpcManager>()->doOnServer(Control::Rpc_StatBlockManager_deleteConn, { connIdTo });
}

void StatBlockManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto statBlockManager = IoC<StatBlockManager>();

    connect(statBlockManager, &StatBlockManager::connChanged, rpcManager,
            [&] { rpcManager->invokeOnClients(Control::Rpc_StatBlockManager_connChanged); });
}
