#include "statconnmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <fortglobal.h>
#include <rpc/rpcmanager.h>
#include <util/variantutil.h>

using namespace Fort;

namespace {

inline bool processStatConnManagerRpcResult(
        StatConnManager *statConnManager, const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_StatConnManager_deleteConn:
        statConnManager->deleteConn(p.args.value(0).toLongLong());
        return true;
    case Control::Rpc_StatConnManager_deleteConns: {
        QVector<qint64> connIds;
        VariantUtil::listToVector(p.args.value(0).toList(), connIds);

        statConnManager->deleteConns(connIds);
        return true;
    }
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

void StatConnManagerRpc::deleteConns(const QVector<qint64> &connIds)
{
    QVariantList connIdVarList;
    VariantUtil::vectorToList(connIds, connIdVarList);

    QVariantList args;
    VariantUtil::addToList(args, QVariant(connIdVarList));

    rpcManager()->doOnServer(Control::Rpc_StatConnManager_deleteConns, args);
}

bool StatConnManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto statConnManager = Fort::statConnManager();

    switch (p.command) {
    case Control::Rpc_StatConnManager_connChanged: {
        emit statConnManager->connChanged();
        return true;
    }
    case Control::Rpc_StatConnManager_connCleared: {
        emit statConnManager->connCleared();
        return true;
    }
    case Control::Rpc_StatConnManager_connIdsShifted: {
        emit statConnManager->connIdsShifted();
        return true;
    }
    case Control::Rpc_StatConnManager_connHostsChanged: {
        emit statConnManager->connHostsChanged();
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
    connect(statConnManager, &StatConnManager::connCleared, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_StatConnManager_connCleared); });
    connect(statConnManager, &StatConnManager::connIdsShifted, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_StatConnManager_connIdsShifted); });
    connect(statConnManager, &StatConnManager::connHostsChanged, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_StatConnManager_connHostsChanged); });
}
