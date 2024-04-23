#include "statmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

bool processStatManager_trafficCleared(StatManager *statManager, const ProcessCommandArgs & /*p*/)
{
    emit statManager->trafficCleared();
    return true;
}

bool processStatManager_appStatRemoved(StatManager *statManager, const ProcessCommandArgs &p)
{
    emit statManager->appStatRemoved(p.args.value(0).toLongLong());
    return true;
}

bool processStatManager_appCreated(StatManager *statManager, const ProcessCommandArgs &p)
{
    emit statManager->appCreated(p.args.value(0).toLongLong(), p.args.value(1).toString());
    return true;
}

bool processStatManager_trafficAdded(StatManager *statManager, const ProcessCommandArgs &p)
{
    emit statManager->trafficAdded(
            p.args.value(0).toLongLong(), p.args.value(1).toUInt(), p.args.value(2).toUInt());
    return true;
}

bool processStatManager_appTrafTotalsResetted(
        StatManager *statManager, const ProcessCommandArgs & /*p*/)
{
    emit statManager->appTrafTotalsResetted();
    return true;
}

using processStatManagerSignal_func = bool (*)(
        StatManager *statManager, const ProcessCommandArgs &p);

static processStatManagerSignal_func processStatManagerSignal_funcList[] = {
    &processStatManager_trafficCleared, // Rpc_StatManager_trafficCleared,
    &processStatManager_appStatRemoved, // Rpc_StatManager_appStatRemoved,
    &processStatManager_appCreated, // Rpc_StatManager_appCreated,
    &processStatManager_trafficAdded, // Rpc_StatManager_trafficAdded,
    &processStatManager_appTrafTotalsResetted, // Rpc_StatManager_appTrafTotalsResetted,
};

inline bool processStatManagerRpcSignal(StatManager *statManager, const ProcessCommandArgs &p)
{
    const processStatManagerSignal_func func = RpcManager::getProcessFunc(p.command,
            processStatManagerSignal_funcList, Control::Rpc_StatManager_trafficCleared,
            Control::Rpc_StatManager_appTrafTotalsResetted);

    return func ? func(statManager, p) : false;
}

inline bool processStatManagerRpcResult(StatManager *statManager, const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_StatManager_deleteStatApp:
        return statManager->deleteStatApp(p.args.value(0).toLongLong());
    case Control::Rpc_StatManager_resetAppTrafTotals:
        return statManager->resetAppTrafTotals();
    case Control::Rpc_StatManager_clearTraffic:
        return statManager->clearTraffic();
    default:
        return false;
    }
}

}

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

bool StatManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, QVariantList & /*resArgs*/, bool &ok, bool &isSendResult)
{
    auto statManager = IoC<StatManager>();

    switch (p.command) {
    case Control::Rpc_StatManager_trafficCleared:
    case Control::Rpc_StatManager_appStatRemoved:
    case Control::Rpc_StatManager_appCreated:
    case Control::Rpc_StatManager_trafficAdded:
    case Control::Rpc_StatManager_appTrafTotalsResetted: {
        return processStatManagerRpcSignal(statManager, p);
    }
    default: {
        ok = processStatManagerRpcResult(statManager, p);
        isSendResult = true;
        return true;
    }
    }
}

void StatManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto statManager = IoC<StatManager>();

    connect(statManager, &StatManager::trafficCleared, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_StatManager_trafficCleared); });
    connect(statManager, &StatManager::appStatRemoved, rpcManager, [=](qint64 appId) {
        rpcManager->invokeOnClients(Control::Rpc_StatManager_appStatRemoved, { appId });
    });
    connect(statManager, &StatManager::appCreated, rpcManager,
            [=](qint64 appId, const QString &appPath) {
                rpcManager->invokeOnClients(
                        Control::Rpc_StatManager_appCreated, { appId, appPath });
            });
    connect(statManager, &StatManager::trafficAdded, rpcManager,
            [=](qint64 unixTime, quint32 inBytes, quint32 outBytes) {
                rpcManager->invokeOnClients(
                        Control::Rpc_StatManager_trafficAdded, { unixTime, inBytes, outBytes });
            });
    connect(statManager, &StatManager::appTrafTotalsResetted, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_StatManager_appTrafTotalsResetted); });
}
