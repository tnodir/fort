#include "statmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

StatManagerRpc::StatManagerRpc(const QString &filePath, FortManager *fortManager, QObject *parent) :
    StatManager(filePath, fortManager->quotaManager(), parent, SqliteDb::OpenDefaultReadOnly),
    m_fortManager(fortManager)
{
}

RpcManager *StatManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

bool StatManagerRpc::deleteStatApp(qint64 appId)
{
    return rpcManager()->doOnServer(Control::Rpc_StatManager_deleteStatApp, { appId });
}

bool StatManagerRpc::deleteConn(qint64 rowIdTo, bool blocked)
{
    return rpcManager()->doOnServer(Control::Rpc_StatManager_deleteConn, { rowIdTo, blocked });
}

bool StatManagerRpc::deleteConnAll()
{
    return rpcManager()->doOnServer(Control::Rpc_StatManager_deleteConnAll);
}

bool StatManagerRpc::resetAppTrafTotals()
{
    return rpcManager()->doOnServer(Control::Rpc_StatManager_resetAppTrafTotals);
}

bool StatManagerRpc::clearTraffic()
{
    return rpcManager()->doOnServer(Control::Rpc_StatManager_clearTraffic);
}

void StatManagerRpc::onConnChanged()
{
    setIsConnIdRangeUpdated(false);

    emit connChanged();
}
