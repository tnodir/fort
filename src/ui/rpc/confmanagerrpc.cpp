#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../conf/firewallconf.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../rpc/rpcmanager.h"
#include "../task/taskmanager.h"

ConfManagerRpc::ConfManagerRpc(const QString &filePath, FortManager *fortManager, QObject *parent) :
    ConfManager(filePath, fortManager, parent, SqliteDb::OpenDefaultReadOnly)
{
}

RpcManager *ConfManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

TaskManager *ConfManagerRpc::taskManager() const
{
    return fortManager()->taskManager();
}

bool ConfManagerRpc::addApp(const QString &appPath, const QString &appName,
        const QDateTime &endTime, int groupIndex, bool useGroupPerm, bool blocked, bool alerted)
{
    Q_ASSERT(!alerted); // Only driver can alert

    return rpcManager()->doOnServer(Control::Rpc_ConfManager_addApp,
            { appPath, appName, endTime, groupIndex, useGroupPerm, blocked });
}

bool ConfManagerRpc::deleteApp(qint64 appId)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_deleteApp, { appId });
}

bool ConfManagerRpc::purgeApps()
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_purgeApps);
}

bool ConfManagerRpc::updateApp(qint64 appId, const QString &appPath, const QString &appName,
        const QDateTime &endTime, int groupIndex, bool useGroupPerm, bool blocked)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_updateApp,
            { appId, appPath, appName, endTime, groupIndex, useGroupPerm, blocked });
}

bool ConfManagerRpc::updateAppBlocked(qint64 appId, bool blocked)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_updateAppBlocked, { appId, blocked });
}

bool ConfManagerRpc::updateAppName(qint64 appId, const QString &appName)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_updateAppName, { appId, appName });
}

bool ConfManagerRpc::addZone(const QString &zoneName, const QString &sourceCode, const QString &url,
        const QString &formData, bool enabled, bool customUrl, int &zoneId)
{
    if (!rpcManager()->doOnServer(Control::Rpc_ConfManager_addZone,
                { zoneName, sourceCode, url, formData, enabled, customUrl }))
        return false;

    zoneId = rpcManager()->resultArgs().value(0).toInt();

    return true;
}

bool ConfManagerRpc::deleteZone(int zoneId)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_deleteZone, { zoneId });
}

bool ConfManagerRpc::updateZone(int zoneId, const QString &zoneName, const QString &sourceCode,
        const QString &url, const QString &formData, bool enabled, bool customUrl)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_updateZone,
            { zoneId, zoneName, sourceCode, url, formData, enabled, customUrl });
}

bool ConfManagerRpc::updateZoneName(int zoneId, const QString &zoneName)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_updateZoneName, { zoneId, zoneName });
}

bool ConfManagerRpc::updateZoneEnabled(int zoneId, bool enabled)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfManager_updateZoneEnabled, { zoneId, enabled });
}

bool ConfManagerRpc::checkPassword(const QString &password)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_checkPassword, { password });
}

bool ConfManagerRpc::saveConf(FirewallConf &newConf)
{
    Q_ASSERT(&newConf == conf() || &newConf == confToEdit()); // else newConf.deleteLater()

    newConf.prepareToSave();

    const QVariant confVar = newConf.toVariant(true);

    setSaving(true);
    const bool ok = rpcManager()->doOnServer(Control::Rpc_ConfManager_save, { confVar });
    setSaving(false);

    if (!ok)
        return false;

    // Already applied by onConfChanged() & applySavedConf()
    newConf.resetEdited();

    return true;
}

void ConfManagerRpc::onConfChanged(const QVariant &confVar)
{
    settings()->clearCache(); // FirewallConf::IniEdited is handled here

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
        taskManager()->loadSettings();
    }

    applySavedConf(conf());

    if (!saving()) {
        fortManager()->reloadOptionsWindow(tr("Settings changed by someone else"));
    }
}
