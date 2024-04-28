#ifndef CONTROL_TYPES_H
#define CONTROL_TYPES_H

#include <QObject>
#include <QVariant>

class ControlWorker;

namespace Control {

enum Command : qint8 {
    CommandNone = 0,

    CommandHome,
    CommandProg,

    Rpc_Result_Ok,
    Rpc_Result_Error,

    Rpc_RpcManager_initClient,

    Rpc_AppInfoManager_lookupAppInfo,
    Rpc_AppInfoManager_checkLookupInfoFinished,

    Rpc_AutoUpdateManager_startDownload,
    Rpc_AutoUpdateManager_runInstaller,
    Rpc_AutoUpdateManager_updateState,
    Rpc_AutoUpdateManager_restartClients,

    Rpc_ConfManager_saveVariant,
    Rpc_ConfManager_exportMasterBackup,
    Rpc_ConfManager_importMasterBackup,
    Rpc_ConfManager_checkPassword,
    Rpc_ConfManager_confChanged,

    Rpc_ConfAppManager_addOrUpdateAppPath,
    Rpc_ConfAppManager_deleteAppPath,
    Rpc_ConfAppManager_addOrUpdateApp,
    Rpc_ConfAppManager_updateApp,
    Rpc_ConfAppManager_updateAppName,
    Rpc_ConfAppManager_deleteApps,
    Rpc_ConfAppManager_purgeApps,
    Rpc_ConfAppManager_updateAppsBlocked,
    Rpc_ConfAppManager_appEndTimesUpdated,
    Rpc_ConfAppManager_appAlerted,
    Rpc_ConfAppManager_appsChanged,
    Rpc_ConfAppManager_appUpdated,

    Rpc_ConfRuleManager_addOrUpdateRule,
    Rpc_ConfRuleManager_deleteRule,
    Rpc_ConfRuleManager_updateRuleName,
    Rpc_ConfRuleManager_updateRuleEnabled,
    Rpc_ConfRuleManager_ruleAdded,
    Rpc_ConfRuleManager_ruleRemoved,
    Rpc_ConfRuleManager_ruleUpdated,

    Rpc_ConfZoneManager_addOrUpdateZone,
    Rpc_ConfZoneManager_deleteZone,
    Rpc_ConfZoneManager_updateZoneName,
    Rpc_ConfZoneManager_updateZoneEnabled,
    Rpc_ConfZoneManager_zoneAdded,
    Rpc_ConfZoneManager_zoneRemoved,
    Rpc_ConfZoneManager_zoneUpdated,

    Rpc_DriverManager_updateState,

    Rpc_QuotaManager_alert,

    Rpc_StatManager_deleteStatApp,
    Rpc_StatManager_resetAppTrafTotals,
    Rpc_StatManager_clearTraffic,
    Rpc_StatManager_trafficCleared,
    Rpc_StatManager_appStatRemoved,
    Rpc_StatManager_appCreated,
    Rpc_StatManager_trafficAdded,
    Rpc_StatManager_appTrafTotalsResetted,

    Rpc_StatBlockManager_deleteConn,
    Rpc_StatBlockManager_connChanged,

    Rpc_ServiceInfoManager_trackService,
    Rpc_ServiceInfoManager_revertService,

    Rpc_TaskManager_runTask,
    Rpc_TaskManager_abortTask,
    Rpc_TaskManager_taskStarted,
    Rpc_TaskManager_taskFinished,
    Rpc_TaskManager_appVersionDownloaded,
    Rpc_TaskManager_zonesDownloaded,
};

enum RpcManager : qint8 {
    Rpc_NoneManager = 0,
    Rpc_AppInfoManager,
    Rpc_AutoUpdateManager,
    Rpc_ConfManager,
    Rpc_ConfAppManager,
    Rpc_ConfRuleManager,
    Rpc_ConfZoneManager,
    Rpc_DriverManager,
    Rpc_QuotaManager,
    Rpc_StatManager,
    Rpc_StatBlockManager,
    Rpc_ServiceInfoManager,
    Rpc_TaskManager,
};

}

struct ProcessCommandArgs
{
    ControlWorker *worker = nullptr;
    Control::Command command = Control::CommandNone;
    const QVariantList &args;
    QString &errorMessage;
};

#endif // CONTROL_TYPES_H
