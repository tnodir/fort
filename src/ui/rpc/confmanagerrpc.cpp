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
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_addApp,
            { appPath, appName, endTime, groupId, groupIndex, useGroupPerm, blocked, alerted });
    return checkResult();
}

bool ConfManagerRpc::deleteApp(qint64 appId, const QString &appPath)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_deleteApp, { appId, appPath });
    return checkResult();
}

bool ConfManagerRpc::updateApp(qint64 appId, const QString &appPath, const QString &appName,
        const QDateTime &endTime, qint64 groupId, int groupIndex, bool useGroupPerm, bool blocked)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_updateApp,
            { appId, appPath, appName, endTime, groupId, groupIndex, useGroupPerm, blocked });
    return checkResult();
}

bool ConfManagerRpc::updateAppName(qint64 appId, const QString &appName)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_updateAppName, { appId, appName });
    return checkResult();
}

bool ConfManagerRpc::addZone(const QString &zoneName, const QString &sourceCode, const QString &url,
        const QString &formData, bool enabled, bool customUrl, int &zoneId)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_addZone,
            { zoneName, sourceCode, url, formData, enabled, customUrl });
    if (!checkResult())
        return false;

    zoneId = rpcManager()->resultArgs().value(0).toInt();

    return true;
}

bool ConfManagerRpc::deleteZone(int zoneId)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_deleteZone, { zoneId });
    return checkResult();
}

bool ConfManagerRpc::updateZone(int zoneId, const QString &zoneName, const QString &sourceCode,
        const QString &url, const QString &formData, bool enabled, bool customUrl)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_updateZone,
            { zoneId, zoneName, sourceCode, url, formData, enabled, customUrl });
    return checkResult();
}

bool ConfManagerRpc::updateZoneName(int zoneId, const QString &zoneName)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_updateZoneName, { zoneId, zoneName });
    return checkResult();
}

bool ConfManagerRpc::updateZoneEnabled(int zoneId, bool enabled)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_updateZoneEnabled, { zoneId, enabled });
    return checkResult();
}

bool ConfManagerRpc::saveConf(FirewallConf &newConf)
{
    setSaving(true);
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_save, { newConf.toVariant() });
    const bool ok = checkResult();
    setSaving(false);

    if (!ok)
        return false;

    if (newConf.iniEdited()) {
        saveClientExtFlags(newConf.ini());
    }

    // Already applied by onConfChanged() & applySavedConf()
    newConf.resetEdited();

    if (&newConf != conf()) {
        newConf.deleteLater();
    }

    return true;
}

bool ConfManagerRpc::checkResult()
{
    if (rpcManager()->waitResult()) {
        showErrorMessage("Service isn't responding.");
        return false;
    }

    if (rpcManager()->resultCommand() != Control::Rpc_Result_Ok) {
        showErrorMessage("Service error.");
        return false;
    }

    return true;
}

void ConfManagerRpc::onConfChanged(const QVariant &confVar)
{
    settings()->clearCache();

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
