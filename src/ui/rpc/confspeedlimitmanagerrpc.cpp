#include "confspeedlimitmanagerrpc.h"

#include <conf/speedlimit.h>
#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

namespace {

bool processConfSpeedLimitManager_addOrUpdateSpeedLimit(
        ConfSpeedLimitManager *confSpeedLimitManager, const ProcessCommandArgs &p,
        ProcessCommandResult &r)
{
    SpeedLimit limit = ConfSpeedLimitManagerRpc::varListToSpeedLimit(p.args);

    const bool ok = confSpeedLimitManager->addOrUpdateSpeedLimit(limit);
    r.args = { limit.limitId };
    return ok;
}

bool processConfSpeedLimitManager_deleteSpeedLimit(ConfSpeedLimitManager *confSpeedLimitManager,
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confSpeedLimitManager->deleteSpeedLimit(p.args.value(0).toUInt());
}

bool processConfSpeedLimitManager_updateSpeedLimitName(ConfSpeedLimitManager *confSpeedLimitManager,
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confSpeedLimitManager->updateSpeedLimitName(
            p.args.value(0).toUInt(), p.args.value(1).toString());
}

bool processConfSpeedLimitManager_updateSpeedLimitEnabled(
        ConfSpeedLimitManager *confSpeedLimitManager, const ProcessCommandArgs &p,
        ProcessCommandResult & /*r*/)
{
    return confSpeedLimitManager->updateSpeedLimitEnabled(
            p.args.value(0).toUInt(), p.args.value(1).toBool());
}

using processConfSpeedLimitManager_func = bool (*)(ConfSpeedLimitManager *confSpeedLimitManager,
        const ProcessCommandArgs &p, ProcessCommandResult &r);

static const processConfSpeedLimitManager_func processConfSpeedLimitManager_funcList[] = {
    &processConfSpeedLimitManager_addOrUpdateSpeedLimit, // Rpc_ConfSpeedLimitManager_addOrUpdateSpeedLimit,
    &processConfSpeedLimitManager_deleteSpeedLimit, // Rpc_ConfSpeedLimitManager_deleteSpeedLimit,
    &processConfSpeedLimitManager_updateSpeedLimitName, // Rpc_ConfSpeedLimitManager_updateSpeedLimitName,
    &processConfSpeedLimitManager_updateSpeedLimitEnabled, // Rpc_ConfSpeedLimitManager_updateSpeedLimitEnabled,
};

inline bool processConfSpeedLimitManagerRpcResult(ConfSpeedLimitManager *confSpeedLimitManager,
        const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processConfSpeedLimitManager_func func =
            RpcManager::getProcessFunc(p.command, processConfSpeedLimitManager_funcList,
                    Control::Rpc_ConfSpeedLimitManager_addOrUpdateSpeedLimit,
                    Control::Rpc_ConfSpeedLimitManager_updateSpeedLimitEnabled);

    return func ? func(confSpeedLimitManager, p, r) : false;
}

}

ConfSpeedLimitManagerRpc::ConfSpeedLimitManagerRpc(QObject *parent) : ConfSpeedLimitManager(parent)
{
}

bool ConfSpeedLimitManagerRpc::addOrUpdateSpeedLimit(SpeedLimit &limit)
{
    QVariantList resArgs;

    if (!rpcManager()->doOnServer(Control::Rpc_ConfSpeedLimitManager_addOrUpdateSpeedLimit,
                speedLimitToVarList(limit), &resArgs))
        return false;

    limit.limitId = resArgs.value(0).toUInt();

    return true;
}

bool ConfSpeedLimitManagerRpc::deleteSpeedLimit(quint8 limitId)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfSpeedLimitManager_deleteSpeedLimit, { limitId });
}

bool ConfSpeedLimitManagerRpc::updateSpeedLimitName(quint8 limitId, const QString &name)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfSpeedLimitManager_updateSpeedLimitName, { limitId, name });
}

bool ConfSpeedLimitManagerRpc::updateSpeedLimitEnabled(quint8 limitId, bool enabled)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfSpeedLimitManager_updateSpeedLimitEnabled, { limitId, enabled });
}

QVariantList ConfSpeedLimitManagerRpc::speedLimitToVarList(const SpeedLimit &limit)
{
    return { limit.enabled, limit.inbound, limit.limitId, limit.packetLoss, limit.latency,
        limit.kbps, limit.bufferSize, limit.name, limit.notes };
}

SpeedLimit ConfSpeedLimitManagerRpc::varListToSpeedLimit(const QVariantList &v)
{
    SpeedLimit limit;
    limit.enabled = v.value(0).toBool();
    limit.inbound = v.value(1).toBool();
    limit.limitId = v.value(2).toUInt();
    limit.packetLoss = v.value(3).toUInt();
    limit.latency = v.value(4).toUInt();
    limit.kbps = v.value(5).toUInt();
    limit.bufferSize = v.value(6).toUInt();
    limit.name = v.value(7).toString();
    limit.notes = v.value(8).toString();
    return limit;
}

bool ConfSpeedLimitManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto confSpeedLimitManager = Fort::confSpeedLimitManager();

    switch (p.command) {
    case Control::Rpc_ConfSpeedLimitManager_speedLimitAdded: {
        emit confSpeedLimitManager->speedLimitAdded();
        return true;
    }
    case Control::Rpc_ConfSpeedLimitManager_speedLimitRemoved: {
        emit confSpeedLimitManager->speedLimitRemoved(p.args.value(0).toUInt());
        return true;
    }
    case Control::Rpc_ConfSpeedLimitManager_speedLimitUpdated: {
        emit confSpeedLimitManager->speedLimitUpdated();
        return true;
    }
    default: {
        r.ok = processConfSpeedLimitManagerRpcResult(confSpeedLimitManager, p, r);
        r.isSendResult = true;
        return true;
    }
    }
}

void ConfSpeedLimitManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confSpeedLimitManager = Fort::confSpeedLimitManager();

    connect(confSpeedLimitManager, &ConfSpeedLimitManager::speedLimitAdded, rpcManager, [=] {
        rpcManager->invokeOnClients(Control::Rpc_ConfSpeedLimitManager_speedLimitAdded);
    });
    connect(confSpeedLimitManager, &ConfSpeedLimitManager::speedLimitRemoved, rpcManager,
            [=](quint8 limitId) {
                rpcManager->invokeOnClients(
                        Control::Rpc_ConfSpeedLimitManager_speedLimitRemoved, { limitId });
            });
    connect(confSpeedLimitManager, &ConfSpeedLimitManager::speedLimitUpdated, rpcManager, [=] {
        rpcManager->invokeOnClients(Control::Rpc_ConfSpeedLimitManager_speedLimitUpdated);
    });
}
