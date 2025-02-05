#include "statconnmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

inline bool processStatConnManagerRpcResult(
        StatConnManager *statConnManager, const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_StatConnManager_deleteConn:
        statConnManager->deleteConn(p.args.value(0).toLongLong());
        return true;
    default:
        return false;
    }
}

}

StatConnManagerRpc::StatConnManagerRpc(const QString &filePath, QObject *parent) :
    StatConnManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

void StatConnManagerRpc::deleteConn(qint64 connIdTo)
{
    IoC<RpcManager>()->doOnServer(Control::Rpc_StatConnManager_deleteConn, { connIdTo });
}

bool StatConnManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, QVariantList & /*resArgs*/, bool &ok, bool &isSendResult)
{
    auto statConnManager = IoC<StatConnManager>();

    switch (p.command) {
    case Control::Rpc_StatConnManager_connChanged: {
        emit statConnManager->connChanged();
        return true;
    }
    default: {
        ok = processStatConnManagerRpcResult(statConnManager, p);
        isSendResult = true;
        return true;
    }
    }
}

void StatConnManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto statConnManager = IoC<StatConnManager>();

    connect(statConnManager, &StatConnManager::connChanged, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_StatConnManager_connChanged); });
}
