#include "pendingconnmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

namespace {

inline bool processPendingConnManagerRpcResult(
        PendingConnManager *pendingConnManager, const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_PendingConnManager_decideConn:
        return pendingConnManager->decideConn(p.args.value(0).toLongLong(),
                PendingConnManager::ConnDecision(p.args.value(1).toInt()));
    case Control::Rpc_PendingConnManager_allowProcess:
        return pendingConnManager->allowProcess(p.args.value(0).toUInt(), p.args.value(1).toUInt());
    default:
        return false;
    }
}

}

PendingConnManagerRpc::PendingConnManagerRpc(const QString &filePath, QObject *parent) :
    PendingConnManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool PendingConnManagerRpc::decideConn(qint64 connId, ConnDecision decision)
{
    return rpcManager()->doOnServer(
            Control::Rpc_PendingConnManager_decideConn, { connId, int(decision) });
}

bool PendingConnManagerRpc::allowProcess(quint32 processId, quint32 seconds)
{
    return rpcManager()->doOnServer(
            Control::Rpc_PendingConnManager_allowProcess, { processId, seconds });
}

bool PendingConnManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto pendingConnManager = Fort::pendingConnManager();

    switch (p.command) {
    case Control::Rpc_PendingConnManager_connChanged: {
        emit pendingConnManager->connChanged();
        return true;
    }
    case Control::Rpc_PendingConnManager_connCleared: {
        emit pendingConnManager->connCleared();
        return true;
    }
    default: {
        r.ok = processPendingConnManagerRpcResult(pendingConnManager, p);
        r.isSendResult = true;
        return true;
    }
    }
}

void PendingConnManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto pendingConnManager = Fort::pendingConnManager();

    connect(pendingConnManager, &PendingConnManager::connChanged, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_PendingConnManager_connChanged); });
    connect(pendingConnManager, &PendingConnManager::connCleared, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_PendingConnManager_connCleared); });
}
