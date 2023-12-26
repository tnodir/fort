#include "confzonemanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <conf/zone.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

ConfZoneManagerRpc::ConfZoneManagerRpc(QObject *parent) : ConfZoneManager(parent) { }

bool ConfZoneManagerRpc::addZone(Zone &zone)
{
    QVariantList resArgs;

    if (!IoC<RpcManager>()->doOnServer(Control::Rpc_ConfZoneManager_addZone,
                { zone.enabled, zone.customUrl, zone.zoneName, zone.sourceCode, zone.url,
                        zone.formData, zone.textInline },
                &resArgs))
        return false;

    zone.zoneId = resArgs.value(0).toInt();

    return true;
}

bool ConfZoneManagerRpc::deleteZone(int zoneId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfZoneManager_deleteZone, { zoneId });
}

bool ConfZoneManagerRpc::updateZone(const Zone &zone)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfZoneManager_updateZone,
            { zone.enabled, zone.customUrl, zone.zoneId, zone.zoneName, zone.sourceCode, zone.url,
                    zone.formData, zone.textInline });
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
