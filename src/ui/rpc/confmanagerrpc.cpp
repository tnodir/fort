#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../conf/firewallconf.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../rpc/rpcmanager.h"

ConfManagerRpc::ConfManagerRpc(const QString &filePath, FortManager *fortManager, QObject *parent) :
    ConfManager(filePath, fortManager, parent, SqliteDb::OpenDefaultReadOnly)
{
}

RpcManager *ConfManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

bool ConfManagerRpc::addApp(const QString &appPath, const QString &appName,
        const QDateTime &endTime, qint64 groupId, int groupIndex, bool useGroupPerm, bool blocked,
        bool alerted)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_addApp,
            { appPath, appName, endTime, groupId, groupIndex, useGroupPerm, blocked, alerted });
}

bool ConfManagerRpc::deleteApp(qint64 appId, const QString &appPath)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_deleteApp, { appId, appPath });
}

bool ConfManagerRpc::updateApp(qint64 appId, const QString &appPath, const QString &appName,
        const QDateTime &endTime, qint64 groupId, int groupIndex, bool useGroupPerm, bool blocked)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_updateApp,
            { appId, appPath, appName, endTime, groupId, groupIndex, useGroupPerm, blocked });
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
    settings()->clearCache();

    const uint editedFlags = FirewallConf::editedFlagsFromVariant(confVar);
    if ((editedFlags & (FirewallConf::OptEdited | FirewallConf::FlagsEdited)) == 0)
        return;

    FirewallConf *newConf = createConf();
    newConf->fromVariant(confVar, true);

    if (newConf->optEdited()) {
        // Reload from storage
        setConf(newConf);
        load();
    } else {
        // Apply only flags
        applySavedConf(newConf);
        delete newConf;
    }

    if (!saving()) {
        fortManager()->reloadOptionsWindow(tr("Settings changed by someone else"));
    }
}
