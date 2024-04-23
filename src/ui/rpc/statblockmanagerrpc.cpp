#include "statblockmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

inline bool processStatBlockManagerRpcResult(
        StatBlockManager *statBlockManager, const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_StatBlockManager_deleteConn:
        statBlockManager->deleteConn(p.args.value(0).toLongLong());
        return true;
    default:
        return false;
    }
}

}

StatBlockManagerRpc::StatBlockManagerRpc(const QString &filePath, QObject *parent) :
    StatBlockManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

void StatBlockManagerRpc::deleteConn(qint64 connIdTo)
{
    IoC<RpcManager>()->doOnServer(Control::Rpc_StatBlockManager_deleteConn, { connIdTo });
}

bool StatBlockManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, QVariantList & /*resArgs*/, bool &ok, bool &isSendResult)
{
    auto statBlockManager = IoC<StatBlockManager>();

    switch (p.command) {
    case Control::Rpc_StatBlockManager_connChanged: {
        emit statBlockManager->connChanged();
        return true;
    }
    default: {
        ok = processStatBlockManagerRpcResult(statBlockManager, p);
        isSendResult = true;
        return true;
    }
    }
}

void StatBlockManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto statBlockManager = IoC<StatBlockManager>();

    connect(statBlockManager, &StatBlockManager::connChanged, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_StatBlockManager_connChanged); });
}
