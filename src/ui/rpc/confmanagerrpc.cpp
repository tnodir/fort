#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <conf/firewallconf.h>
#include <conf/zone.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/rpcmanager.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/variantutil.h>

ConfManagerRpc::ConfManagerRpc(const QString &filePath, QObject *parent) :
    ConfManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool ConfManagerRpc::exportBackup(const QString &path)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_exportBackup, { path });
}

bool ConfManagerRpc::importBackup(const QString &path)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_importBackup, { path });
}

bool ConfManagerRpc::addApp(const App &app)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_addApp, appToVarList(app));
}

void ConfManagerRpc::deleteApps(const QVector<qint64> &appIdList)
{
    const QVariantList appIdVarList = VariantUtil::vectorToList(appIdList);

    QVariantList args;
    VariantUtil::addToList(args, appIdVarList);

    IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_deleteApps, args);
}

bool ConfManagerRpc::purgeApps()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_purgeApps);
}

bool ConfManagerRpc::updateApp(const App &app)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_updateApp, appToVarList(app));
}

void ConfManagerRpc::updateAppsBlocked(
        const QVector<qint64> &appIdList, bool blocked, bool killProcess)
{
    const QVariantList appIdVarList = VariantUtil::vectorToList(appIdList);

    QVariantList args;
    VariantUtil::addToList(args, appIdVarList);
    args << blocked << killProcess;

    IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_updateAppsBlocked, args);
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
                        zone.formData, zone.textInline },
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
                    zone.formData, zone.textInline });
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

    const QVariant confVar = newConf.toVariant(/*onlyEdited=*/true);

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
        conf()->fromVariant(confVar, /*onlyEdited=*/true);
    }

    if ((editedFlags & FirewallConf::TaskEdited) != 0) {
        IoC<TaskManager>()->loadSettings();
    }

    applySavedConf(conf());

    if (!saving()) {
        IoC<WindowManager>()->reloadOptionsWindow(tr("Settings changed by someone else"));
    }
}

QVariantList ConfManagerRpc::appToVarList(const App &app)
{
    return { app.isWildcard, app.useGroupPerm, app.applyChild, app.killChild, app.lanOnly,
        app.logBlocked, app.logConn, app.blocked, app.killProcess, app.groupIndex, app.acceptZones,
        app.rejectZones, app.appId, app.appOriginPath, app.appPath, app.appName, app.endTime };
}

App ConfManagerRpc::varListToApp(const QVariantList &v)
{
    App app;
    app.isWildcard = v.value(0).toBool();
    app.useGroupPerm = v.value(1).toBool();
    app.applyChild = v.value(2).toBool();
    app.killChild = v.value(3).toBool();
    app.lanOnly = v.value(4).toBool();
    app.logBlocked = v.value(5).toBool();
    app.logConn = v.value(6).toBool();
    app.blocked = v.value(7).toBool();
    app.killProcess = v.value(8).toBool();
    app.groupIndex = v.value(9).toInt();
    app.acceptZones = v.value(10).toUInt();
    app.rejectZones = v.value(11).toUInt();
    app.appId = v.value(12).toLongLong();
    app.appOriginPath = v.value(13).toString();
    app.appPath = v.value(14).toString();
    app.appName = v.value(15).toString();
    app.endTime = v.value(16).toDateTime();
    return app;
}
