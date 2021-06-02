#include "statmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"
#include "../util/ioc/ioccontainer.h"

StatManagerRpc::StatManagerRpc(const QString &filePath, QObject *parent) :
    StatManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool StatManagerRpc::deleteStatApp(qint64 appId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatManager_deleteStatApp, { appId });
}

bool StatManagerRpc::deleteConn(qint64 rowIdTo, bool blocked)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatManager_deleteConn, { rowIdTo, blocked });
}

bool StatManagerRpc::deleteConnAll()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatManager_deleteConnAll);
}

bool StatManagerRpc::resetAppTrafTotals()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatManager_resetAppTrafTotals);
}

bool StatManagerRpc::clearTraffic()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatManager_clearTraffic);
}

void StatManagerRpc::onConnChanged()
{
    setIsConnIdRangeUpdated(false);

    emit connChanged();
}
