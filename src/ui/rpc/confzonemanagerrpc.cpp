#include "confzonemanagerrpc.h"

#include <conf/zone.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

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

void ConfZoneManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confZoneManager = IoC<ConfZoneManager>();

    connect(confZoneManager, &ConfZoneManager::zoneAdded, rpcManager,
            [&] { rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneAdded); });
    connect(confZoneManager, &ConfZoneManager::zoneRemoved, rpcManager, [&](int zoneId) {
        rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneRemoved, { zoneId });
    });
    connect(confZoneManager, &ConfZoneManager::zoneUpdated, rpcManager,
            [&] { rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneUpdated); });
}
