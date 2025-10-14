#include "statconnmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

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
    rpcManager()->doOnServer(Control::Rpc_StatConnManager_deleteConn, { connIdTo });
}

bool StatConnManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto statConnManager = Fort::statConnManager();

    switch (p.command) {
    case Control::Rpc_StatConnManager_connChanged: {
        emit statConnManager->connChanged();
        return true;
    }
    default: {
        r.ok = processStatConnManagerRpcResult(statConnManager, p);
        r.isSendResult = true;
        return true;
    }
    }
}

void StatConnManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto statConnManager = Fort::statConnManager();

    connect(statConnManager, &StatConnManager::connChanged, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_StatConnManager_connChanged); });
}
