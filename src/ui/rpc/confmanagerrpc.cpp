#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <conf/firewallconf.h>
#include <conf/zone.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/rpcmanager.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>

ConfManagerRpc::ConfManagerRpc(const QString &filePath, QObject *parent) :
    ConfManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool ConfManagerRpc::addApp(const App &app)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_addApp,
            { app.isWildcard, app.useGroupPerm, app.applyChild, app.lanOnly, app.logBlocked,
                    app.logConn, app.blocked, app.killProcess, app.groupIndex, app.appOriginPath,
                    app.appPath, app.appName, app.endTime });
}

bool ConfManagerRpc::deleteApp(qint64 appId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_deleteApp, { appId });
}

bool ConfManagerRpc::purgeApps()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_purgeApps);
}

bool ConfManagerRpc::updateApp(const App &app)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_updateApp,
            { app.isWildcard, app.useGroupPerm, app.applyChild, app.lanOnly, app.logBlocked,
                    app.logConn, app.blocked, app.killProcess, app.groupIndex, app.appId,
                    app.appOriginPath, app.appPath, app.appName, app.endTime });
}

bool ConfManagerRpc::updateAppBlocked(qint64 appId, bool blocked, bool killProcess)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfManager_updateAppBlocked, { appId, blocked, killProcess });
}

bool ConfManagerRpc::updateAppName(qint64 appId, const QString &appName)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfManager_updateAppName, { appId, appName });
}

bool ConfManagerRpc::addZone(Zone &zone)
{
    QVariantList resArgs;

    if (!IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_addZone,
                { zone.enabled, zone.customUrl, zone.zoneName, zone.sourceCode, zone.url,
                        zone.formData },
                &resArgs))
        return false;

    zone.zoneId = resArgs.value(0).toInt();

    return true;
}

bool ConfManagerRpc::deleteZone(int zoneId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_deleteZone, { zoneId });
}

bool ConfManagerRpc::updateZone(const Zone &zone)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_updateZone,
            { zone.enabled, zone.customUrl, zone.zoneId, zone.zoneName, zone.sourceCode, zone.url,
                    zone.formData });
}

bool ConfManagerRpc::updateZoneName(int zoneId, const QString &zoneName)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfManager_updateZoneName, { zoneId, zoneName });
}

bool ConfManagerRpc::updateZoneEnabled(int zoneId, bool enabled)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfManager_updateZoneEnabled, { zoneId, enabled });
}

bool ConfManagerRpc::checkPassword(const QString &password)
{
    QVariantList resArgs;

    if (!IoC<RpcManager>()->doOnServer(
                Control::Rpc_ConfManager_checkPassword, { password }, &resArgs))
        return false;

    return resArgs.value(0).toBool();
}

bool ConfManagerRpc::saveConf(FirewallConf &newConf)
{
    Q_ASSERT(&newConf == conf() || &newConf == confToEdit()); // else newConf.deleteLater()

    newConf.prepareToSave();

    const QVariant confVar = newConf.toVariant(true);

    setSaving(true);
    const bool ok =
            IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_saveVariant, { confVar });
    setSaving(false);

    if (!ok)
        return false;

    // Already applied by onConfChanged() & applySavedConf()
    newConf.resetEdited();

    if (&newConf == confToEdit()) {
        setConfToEdit(nullptr);
    }

    return true;
}

void ConfManagerRpc::onConfChanged(const QVariant &confVar)
{
    IoC<FortSettings>()->clearCache(); // FirewallConf::IniEdited is handled here

    const uint editedFlags = FirewallConf::editedFlagsFromVariant(confVar);

    if ((editedFlags & FirewallConf::OptEdited) != 0) {
        // Reload from storage
        setConf(createConf());
        loadConf(*conf());
    } else {
        // Apply only flags
        conf()->fromVariant(confVar, true);
    }

    if ((editedFlags & FirewallConf::TaskEdited) != 0) {
        IoC<TaskManager>()->loadSettings();
    }

    applySavedConf(conf());

    if (!saving()) {
        IoC<WindowManager>()->reloadOptionsWindow(tr("Settings changed by someone else"));
    }
}
