#include "confzonemanagerrpc.h"

#include <conf/zone.h>
#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

namespace {

bool processConfZoneManager_addOrUpdateZone(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    Zone zone = ConfZoneManagerRpc::varListToZone(p.args);

    const bool ok = confZoneManager->addOrUpdateZone(zone);
    r.args = { zone.zoneId };
    return ok;
}

bool processConfZoneManager_deleteZone(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confZoneManager->deleteZone(p.args.value(0).toLongLong());
}

bool processConfZoneManager_updateZoneName(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confZoneManager->updateZoneName(
            p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfZoneManager_updateZoneEnabled(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confZoneManager->updateZoneEnabled(
            p.args.value(0).toLongLong(), p.args.value(1).toBool());
}

using processConfZoneManager_func = bool (*)(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, ProcessCommandResult &r);

static const processConfZoneManager_func processConfZoneManager_funcList[] = {
    &processConfZoneManager_addOrUpdateZone, // Rpc_ConfZoneManager_addOrUpdateZone,
    &processConfZoneManager_deleteZone, // Rpc_ConfZoneManager_deleteZone,
    &processConfZoneManager_updateZoneName, // Rpc_ConfZoneManager_updateZoneName,
    &processConfZoneManager_updateZoneEnabled, // Rpc_ConfZoneManager_updateZoneEnabled,
};

inline bool processConfZoneManagerRpcResult(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processConfZoneManager_func func = RpcManager::getProcessFunc(p.command,
            processConfZoneManager_funcList, Control::Rpc_ConfZoneManager_addOrUpdateZone,
            Control::Rpc_ConfZoneManager_updateZoneEnabled);

    return func ? func(confZoneManager, p, r) : false;
}

}

ConfZoneManagerRpc::ConfZoneManagerRpc(QObject *parent) : ConfZoneManager(parent) { }

bool ConfZoneManagerRpc::addOrUpdateZone(Zone &zone)
{
    QVariantList resArgs;

    if (!rpcManager()->doOnServer(
                Control::Rpc_ConfZoneManager_addOrUpdateZone, zoneToVarList(zone), &resArgs))
        return false;

    zone.zoneId = resArgs.value(0).toInt();

    return true;
}

bool ConfZoneManagerRpc::deleteZone(quint8 zoneId)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfZoneManager_deleteZone, { zoneId });
}

bool ConfZoneManagerRpc::updateZoneName(quint8 zoneId, const QString &zoneName)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfZoneManager_updateZoneName, { zoneId, zoneName });
}

bool ConfZoneManagerRpc::updateZoneEnabled(quint8 zoneId, bool enabled)
{
    return rpcManager()->doOnServer(
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

bool ConfZoneManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto confZoneManager = Fort::confZoneManager();

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
        r.ok = processConfZoneManagerRpcResult(confZoneManager, p, r);
        r.isSendResult = true;
        return true;
    }
    }
}

void ConfZoneManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confZoneManager = Fort::confZoneManager();

    connect(confZoneManager, &ConfZoneManager::zoneAdded, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneAdded); });
    connect(confZoneManager, &ConfZoneManager::zoneRemoved, rpcManager, [=](quint8 zoneId) {
        rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneRemoved, { zoneId });
    });
    connect(confZoneManager, &ConfZoneManager::zoneUpdated, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfZoneManager_zoneUpdated); });
}
