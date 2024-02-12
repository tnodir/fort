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

void StatManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto statManager = IoC<StatManager>();

    connect(statManager, &StatManager::trafficCleared, rpcManager,
            [&] { rpcManager->invokeOnClients(Control::Rpc_StatManager_trafficCleared); });
    connect(statManager, &StatManager::appStatRemoved, rpcManager, [&](qint64 appId) {
        rpcManager->invokeOnClients(Control::Rpc_StatManager_appStatRemoved, { appId });
    });
    connect(statManager, &StatManager::appCreated, rpcManager,
            [&](qint64 appId, const QString &appPath) {
                rpcManager->invokeOnClients(
                        Control::Rpc_StatManager_appCreated, { appId, appPath });
            });
    connect(statManager, &StatManager::trafficAdded, rpcManager,
            [&](qint64 unixTime, quint32 inBytes, quint32 outBytes) {
                rpcManager->invokeOnClients(
                        Control::Rpc_StatManager_trafficAdded, { unixTime, inBytes, outBytes });
            });
    connect(statManager, &StatManager::appTrafTotalsResetted, rpcManager,
            [&] { rpcManager->invokeOnClients(Control::Rpc_StatManager_appTrafTotalsResetted); });
}
