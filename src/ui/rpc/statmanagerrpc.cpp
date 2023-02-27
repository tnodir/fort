#include "statmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

StatManagerRpc::StatManagerRpc(const QString &filePath, QObject *parent) :
    StatManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool StatManagerRpc::deleteStatApp(qint64 appId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatManager_deleteStatApp, { appId });
}

bool StatManagerRpc::resetAppTrafTotals()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatManager_resetAppTrafTotals);
}

bool StatManagerRpc::clearTraffic()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_StatManager_clearTraffic);
}
