#include "confzonemanagerrpc.h"

#include <conf/zone.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

bool processConfZoneManager_addOrUpdateZone(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    Zone zone = ConfZoneManagerRpc::varListToZone(p.args);

    const bool ok = confZoneManager->addOrUpdateZone(zone);
    resArgs = { zone.zoneId };
    return ok;
}

bool processConfZoneManager_deleteZone(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confZoneManager->deleteZone(p.args.value(0).toLongLong());
}

bool processConfZoneManager_updateZoneName(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confZoneManager->updateZoneName(
            p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfZoneManager_updateZoneEnabled(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confZoneManager->updateZoneEnabled(
            p.args.value(0).toLongLong(), p.args.value(1).toBool());
}

using processConfZoneManager_func = bool (*)(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processConfZoneManager_func processConfZoneManager_funcList[] = {
    &processConfZoneManager_addOrUpdateZone, // Rpc_ConfZoneManager_addOrUpdateZone,
    &processConfZoneManager_deleteZone, // Rpc_ConfZoneManager_deleteZone,
    &processConfZoneManager_updateZoneName, // Rpc_ConfZoneManager_updateZoneName,
    &processConfZoneManager_updateZoneEnabled, // Rpc_ConfZoneManager_updateZoneEnabled,
};

inline bool processConfZoneManagerRpcResult(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const processConfZoneManager_func func = RpcManager::getProcessFunc(p.command,
            processConfZoneManager_funcList, Control::Rpc_ConfZoneManager_addOrUpdateZone,
            Control::Rpc_ConfZoneManager_updateZoneEnabled);

    return func ? func(confZoneManager, p, resArgs) : false;
}

}

ConfZoneManagerRpc::ConfZoneManagerRpc(QObject *parent) : ConfZoneManager(parent) { }

bool ConfZoneManagerRpc::addOrUpdateZone(Zone &zone)
{
    QVariantList resArgs;

    if (!IoC<RpcManager>()->doOnServer(
                Control::Rpc_ConfZoneManager_addOrUpdateZone, zoneToVarList(zone), &resArgs))
        return false;

    zone.zoneId = resArgs.value(0).toInt();

    return true;
}

bool ConfZoneManagerRpc::deleteZone(int zoneId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfZoneManager_deleteZone, { zoneId });
}

bool ConfZoneManagerRpc::updateZoneName(int zoneId, const QString &zoneName)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfZoneManager_updateZoneName, { zoneId, zoneName });
}

bool ConfZoneManagerRpc::updateZoneEnabled(int zoneId, bool enabled)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfZoneManager_updateZoneEnabled, { zoneId, enabled });
}

QVariantList ConfZoneManagerRpc::zoneToVarList(const Zone &zone)
{
    return { zone.enabled, zone.customUrl, zone.zoneId, zone.zoneName, zone.sourceCode, zone.url,
        zone.formData, zone.textInline };
}

Zone ConfZoneManagerRpc::varListToZone(const QVariantList &v)
{
    Zone zone;
    zone.enabled = v.value(0).toBool();
    zone.customUrl = v.value(1).toBool();
    zone.zoneId = v.value(2).toInt();
    zone.zoneName = v.value(3).toString();
    zone.sourceCode = v.value(4).toString();
    zone.url = v.value(5).toString();
    zone.formData = v.value(6).toString();
    zone.textInline = v.value(7).toString();
    return zone;
}

bool ConfZoneManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult)
{
    auto confZoneManager = IoC<ConfZoneManager>();

    switch (p.command) {
    case Control::Rpc_ConfZoneManager_zoneAdded: {
        emit confZoneManager->zoneAdded();
        return true;
    }
    case Control::Rpc_ConfZoneManager_zoneRemoved: {
        emit confZoneManager->zoneRemoved(p.args.value(0).toInt());
        return true;
    }
    case Control::Rpc_ConfZoneManager_zoneUpdated: {
        emit confZoneManager->zoneUpdated();
        return true;
    }
    default: {
        ok = processConfZoneManagerRpcResult(confZoneManager, p, resArgs);
        isSendResult = true;
        return true;
    }
    }
}

void ConfZoneManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confZoneManager = IoC<ConfZoneManager>();

    connect(confZoneManager, &ConfZoneManager::zoneAdded, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneAdded); });
    connect(confZoneManager, &ConfZoneManager::zoneRemoved, rpcManager, [=](int zoneId) {
        rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneRemoved, { zoneId });
    });
    connect(confZoneManager, &ConfZoneManager::zoneUpdated, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneUpdated); });
}
