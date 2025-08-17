#include "control.h"

#include <QHash>

namespace Control {

#define CASE_STRING(id) { id, #id }

static const QHash<Command, const char *> g_commandNames = {
    CASE_STRING(CommandNone),

    CASE_STRING(CommandHome),
    CASE_STRING(CommandFilter),
    CASE_STRING(CommandFilterMode),
    CASE_STRING(CommandBlock),
    CASE_STRING(CommandProg),
    CASE_STRING(CommandGroup),
    CASE_STRING(CommandConf),
    CASE_STRING(CommandBackup),
    CASE_STRING(CommandZone),

    CASE_STRING(Rpc_Result_Ok),
    CASE_STRING(Rpc_Result_Error),

    CASE_STRING(Rpc_RpcManager_initClient),

    CASE_STRING(Rpc_AppInfoManager_lookupAppInfo),
    CASE_STRING(Rpc_AppInfoManager_checkLookupInfoFinished),

    CASE_STRING(Rpc_AutoUpdateManager_startDownload),
    CASE_STRING(Rpc_AutoUpdateManager_runInstaller),
    CASE_STRING(Rpc_AutoUpdateManager_updateState),
    CASE_STRING(Rpc_AutoUpdateManager_restartClients),

    CASE_STRING(Rpc_ConfManager_saveVariant),
    CASE_STRING(Rpc_ConfManager_exportMasterBackup),
    CASE_STRING(Rpc_ConfManager_importMasterBackup),
    CASE_STRING(Rpc_ConfManager_checkPassword),
    CASE_STRING(Rpc_ConfManager_confChanged),
    CASE_STRING(Rpc_ConfManager_imported),

    CASE_STRING(Rpc_ConfAppManager_addOrUpdateAppPath),
    CASE_STRING(Rpc_ConfAppManager_deleteAppPath),
    CASE_STRING(Rpc_ConfAppManager_addOrUpdateApp),
    CASE_STRING(Rpc_ConfAppManager_updateApp),
    CASE_STRING(Rpc_ConfAppManager_updateAppName),
    CASE_STRING(Rpc_ConfAppManager_deleteApps),
    CASE_STRING(Rpc_ConfAppManager_clearAlerts),
    CASE_STRING(Rpc_ConfAppManager_purgeApps),
    CASE_STRING(Rpc_ConfAppManager_updateAppsBlocked),
    CASE_STRING(Rpc_ConfAppManager_updateAppsTimer),
    CASE_STRING(Rpc_ConfAppManager_importAppsBackup),
    CASE_STRING(Rpc_ConfAppManager_updateDriverConf),
    CASE_STRING(Rpc_ConfAppManager_appAlerted),
    CASE_STRING(Rpc_ConfAppManager_appsChanged),
    CASE_STRING(Rpc_ConfAppManager_appUpdated),
    CASE_STRING(Rpc_ConfAppManager_appDeleted),

    CASE_STRING(Rpc_ConfRuleManager_addOrUpdateRule),
    CASE_STRING(Rpc_ConfRuleManager_deleteRule),
    CASE_STRING(Rpc_ConfRuleManager_updateRuleName),
    CASE_STRING(Rpc_ConfRuleManager_updateRuleEnabled),
    CASE_STRING(Rpc_ConfRuleManager_ruleAdded),
    CASE_STRING(Rpc_ConfRuleManager_ruleRemoved),
    CASE_STRING(Rpc_ConfRuleManager_ruleUpdated),
    CASE_STRING(Rpc_ConfRuleManager_trayMenuUpdated),

    CASE_STRING(Rpc_ConfZoneManager_addOrUpdateZone),
    CASE_STRING(Rpc_ConfZoneManager_deleteZone),
    CASE_STRING(Rpc_ConfZoneManager_updateZoneName),
    CASE_STRING(Rpc_ConfZoneManager_updateZoneEnabled),
    CASE_STRING(Rpc_ConfZoneManager_zoneAdded),
    CASE_STRING(Rpc_ConfZoneManager_zoneRemoved),
    CASE_STRING(Rpc_ConfZoneManager_zoneUpdated),

    CASE_STRING(Rpc_DriverManager_updateState),

    CASE_STRING(Rpc_QuotaManager_alert),

    CASE_STRING(Rpc_StatManager_deleteStatApp),
    CASE_STRING(Rpc_StatManager_resetAppTrafTotals),
    CASE_STRING(Rpc_StatManager_exportMasterBackup),
    CASE_STRING(Rpc_StatManager_importMasterBackup),
    CASE_STRING(Rpc_StatManager_clearTraffic),
    CASE_STRING(Rpc_StatManager_trafficCleared),
    CASE_STRING(Rpc_StatManager_appStatRemoved),
    CASE_STRING(Rpc_StatManager_appCreated),
    CASE_STRING(Rpc_StatManager_trafficAdded),
    CASE_STRING(Rpc_StatManager_appTrafTotalsResetted),

    CASE_STRING(Rpc_StatConnManager_deleteConn),
    CASE_STRING(Rpc_StatConnManager_connChanged),

    CASE_STRING(Rpc_ServiceInfoManager_trackService),
    CASE_STRING(Rpc_ServiceInfoManager_revertService),

    CASE_STRING(Rpc_TaskManager_runTask),
    CASE_STRING(Rpc_TaskManager_abortTask),
    CASE_STRING(Rpc_TaskManager_taskStarted),
    CASE_STRING(Rpc_TaskManager_taskFinished),
    CASE_STRING(Rpc_TaskManager_appVersionUpdated),
    CASE_STRING(Rpc_TaskManager_appVersionDownloaded),
    CASE_STRING(Rpc_TaskManager_zonesDownloaded),
};

const char *const commandString(Command cmd)
{
    return g_commandNames.value(cmd);
}

static const QHash<RpcManager, const char *> g_managerNames = {
    CASE_STRING(Rpc_NoneManager),
    CASE_STRING(Rpc_AppInfoManager),
    CASE_STRING(Rpc_AutoUpdateManager),
    CASE_STRING(Rpc_ConfManager),
    CASE_STRING(Rpc_ConfAppManager),
    CASE_STRING(Rpc_ConfRuleManager),
    CASE_STRING(Rpc_ConfZoneManager),
    CASE_STRING(Rpc_DriverManager),
    CASE_STRING(Rpc_QuotaManager),
    CASE_STRING(Rpc_StatManager),
    CASE_STRING(Rpc_StatConnManager),
    CASE_STRING(Rpc_ServiceInfoManager),
    CASE_STRING(Rpc_TaskManager),
};

const char *const rpcManagerString(RpcManager rpcManager)
{
    return g_managerNames.value(rpcManager);
}

static const RpcManager g_commandManagers[] = {
    Rpc_NoneManager, // CommandNone = 0,

    Rpc_NoneManager, // CommandHome,
    Rpc_NoneManager, // CommandFilter,
    Rpc_NoneManager, // CommandFilterMode,
    Rpc_NoneManager, // CommandBlock,
    Rpc_NoneManager, // CommandProg,
    Rpc_NoneManager, // CommandGroup,
    Rpc_NoneManager, // CommandConf,
    Rpc_NoneManager, // CommandBackup,
    Rpc_NoneManager, // CommandZone,

    Rpc_NoneManager, // Rpc_Result_Ok,
    Rpc_NoneManager, // Rpc_Result_Error,

    Rpc_NoneManager, // Rpc_RpcManager_initClient,

    Rpc_AppInfoManager, // Rpc_AppInfoManager_lookupAppInfo,
    Rpc_AppInfoManager, // Rpc_AppInfoManager_checkLookupFinished,

    Rpc_AutoUpdateManager, // Rpc_AutoUpdateManager_startDownload,
    Rpc_AutoUpdateManager, // Rpc_AutoUpdateManager_runInstaller,
    Rpc_AutoUpdateManager, // Rpc_AutoUpdateManager_updateState,
    Rpc_AutoUpdateManager, // Rpc_AutoUpdateManager_restartClients,

    Rpc_ConfManager, // Rpc_ConfManager_saveVariant,
    Rpc_ConfManager, // Rpc_ConfManager_exportMasterBackup,
    Rpc_ConfManager, // Rpc_ConfManager_importMasterBackup,
    Rpc_ConfManager, // Rpc_ConfManager_checkPassword,
    Rpc_ConfManager, // Rpc_ConfManager_confChanged,
    Rpc_ConfManager, // Rpc_ConfManager_imported,

    Rpc_ConfAppManager, // Rpc_ConfAppManager_addOrUpdateAppPath,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_deleteAppPath,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_addOrUpdateApp,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_updateApp,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_updateAppName,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_deleteApps,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_clearAlerts,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_purgeApps,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_updateAppsBlocked,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_updateAppsTimer,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_importAppsBackup,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_updateDriverConf,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_appEndTimesUpdated,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_appAlerted,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_appsChanged,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_appUpdated,
    Rpc_ConfAppManager, // Rpc_ConfAppManager_appDeleted,

    Rpc_ConfRuleManager, // Rpc_ConfRuleManager_addOrUpdateRule,
    Rpc_ConfRuleManager, // Rpc_ConfRuleManager_deleteRule,
    Rpc_ConfRuleManager, // Rpc_ConfRuleManager_updateRuleName,
    Rpc_ConfRuleManager, // Rpc_ConfRuleManager_updateRuleEnabled,
    Rpc_ConfRuleManager, // Rpc_ConfRuleManager_ruleAdded,
    Rpc_ConfRuleManager, // Rpc_ConfRuleManager_ruleRemoved,
    Rpc_ConfRuleManager, // Rpc_ConfRuleManager_ruleUpdated,
    Rpc_ConfRuleManager, // Rpc_ConfRuleManager_trayMenuUpdated,

    Rpc_ConfZoneManager, // Rpc_ConfZoneManager_addOrUpdateZone,
    Rpc_ConfZoneManager, // Rpc_ConfZoneManager_deleteZone,
    Rpc_ConfZoneManager, // Rpc_ConfZoneManager_updateZoneName,
    Rpc_ConfZoneManager, // Rpc_ConfZoneManager_updateZoneEnabled,
    Rpc_ConfZoneManager, // Rpc_ConfZoneManager_zoneAdded,
    Rpc_ConfZoneManager, // Rpc_ConfZoneManager_zoneRemoved,
    Rpc_ConfZoneManager, // Rpc_ConfZoneManager_zoneUpdated,

    Rpc_DriverManager, // Rpc_DriverManager_updateState,

    Rpc_QuotaManager, // Rpc_QuotaManager_alert,

    Rpc_StatManager, // Rpc_StatManager_deleteStatApp,
    Rpc_StatManager, // Rpc_StatManager_resetAppTrafTotals,
    Rpc_StatManager, // Rpc_StatManager_exportMasterBackup,
    Rpc_StatManager, // Rpc_StatManager_importMasterBackup,
    Rpc_StatManager, // Rpc_StatManager_clearTraffic,
    Rpc_StatManager, // Rpc_StatManager_trafficCleared,
    Rpc_StatManager, // Rpc_StatManager_appStatRemoved,
    Rpc_StatManager, // Rpc_StatManager_appCreated,
    Rpc_StatManager, // Rpc_StatManager_trafficAdded,
    Rpc_StatManager, // Rpc_StatManager_appTrafTotalsResetted,

    Rpc_StatConnManager, // Rpc_StatConnManager_deleteConn,
    Rpc_StatConnManager, // Rpc_StatConnManager_connChanged,

    Rpc_ServiceInfoManager, // Rpc_ServiceInfoManager_trackService,
    Rpc_ServiceInfoManager, // Rpc_ServiceInfoManager_revertService,

    Rpc_TaskManager, // Rpc_TaskManager_runTask,
    Rpc_TaskManager, // Rpc_TaskManager_abortTask,
    Rpc_TaskManager, // Rpc_TaskManager_taskStarted,
    Rpc_TaskManager, // Rpc_TaskManager_taskFinished,
    Rpc_TaskManager, // Rpc_TaskManager_appVersionUpdated,
    Rpc_TaskManager, // Rpc_TaskManager_appVersionDownloaded,
    Rpc_TaskManager, // Rpc_TaskManager_zonesDownloaded,
};

RpcManager managerByCommand(Command cmd)
{
    return g_commandManagers[cmd];
}

static const qint8 g_commandValidations[] = {
    0, // CommandNone = 0,

    0, // CommandHome,
    0, // CommandFilter,
    0, // CommandFilterMode,
    0, // CommandBlock,
    0, // CommandProg,
    0, // CommandGroup,
    0, // CommandConf,
    0, // CommandBackup,
    0, // CommandZone,

    0, // Rpc_Result_Ok,
    0, // Rpc_Result_Error,

    0, // Rpc_RpcManager_initClient,

    true, // Rpc_AppInfoManager_lookupAppInfo,
    0, // Rpc_AppInfoManager_checkLookupFinished,

    true, // Rpc_AutoUpdateManager_startDownload,
    true, // Rpc_AutoUpdateManager_runInstaller,
    0, // Rpc_AutoUpdateManager_updateState,
    0, // Rpc_AutoUpdateManager_restartClients,

    true, // Rpc_ConfManager_saveVariant,
    true, // Rpc_ConfManager_exportMasterBackup,
    true, // Rpc_ConfManager_importMasterBackup,
    0, // Rpc_ConfManager_checkPassword,
    0, // Rpc_ConfManager_confChanged,
    0, // Rpc_ConfManager_imported,

    true, // Rpc_ConfAppManager_addOrUpdateAppPath,
    true, // Rpc_ConfAppManager_deleteAppPath,
    true, // Rpc_ConfAppManager_addOrUpdateApp,
    true, // Rpc_ConfAppManager_updateApp,
    true, // Rpc_ConfAppManager_updateAppName,
    true, // Rpc_ConfAppManager_deleteApps,
    true, // Rpc_ConfAppManager_clearAlerts,
    true, // Rpc_ConfAppManager_purgeApps,
    true, // Rpc_ConfAppManager_updateAppsBlocked,
    true, // Rpc_ConfAppManager_updateAppsTimer,
    true, // Rpc_ConfAppManager_importAppsBackup,
    true, // Rpc_ConfAppManager_updateDriverConf,
    0, // Rpc_ConfAppManager_appEndTimesUpdated,
    0, // Rpc_ConfAppManager_appAlerted,
    0, // Rpc_ConfAppManager_appsChanged,
    0, // Rpc_ConfAppManager_appUpdated,
    0, // Rpc_ConfAppManager_appDeleted,

    true, // Rpc_ConfRuleManager_addOrUpdateRule,
    true, // Rpc_ConfRuleManager_deleteRule,
    true, // Rpc_ConfRuleManager_updateRuleName,
    true, // Rpc_ConfRuleManager_updateRuleEnabled,
    0, // Rpc_ConfRuleManager_ruleAdded,
    0, // Rpc_ConfRuleManager_ruleRemoved,
    0, // Rpc_ConfRuleManager_ruleUpdated,
    0, // Rpc_ConfRuleManager_trayMenuUpdated,

    true, // Rpc_ConfZoneManager_addOrUpdateZone,
    true, // Rpc_ConfZoneManager_deleteZone,
    true, // Rpc_ConfZoneManager_updateZoneName,
    true, // Rpc_ConfZoneManager_updateZoneEnabled,
    0, // Rpc_ConfZoneManager_zoneAdded,
    0, // Rpc_ConfZoneManager_zoneRemoved,
    0, // Rpc_ConfZoneManager_zoneUpdated,

    0, // Rpc_DriverManager_updateState,

    0, // Rpc_QuotaManager_alert,

    true, // Rpc_StatManager_deleteStatApp,
    true, // Rpc_StatManager_resetAppTrafTotals,
    true, // Rpc_StatManager_exportMasterBackup,
    true, // Rpc_StatManager_importMasterBackup,
    true, // Rpc_StatManager_clearTraffic,
    0, // Rpc_StatManager_trafficCleared,
    0, // Rpc_StatManager_appStatRemoved,
    0, // Rpc_StatManager_appCreated,
    0, // Rpc_StatManager_trafficAdded,
    0, // Rpc_StatManager_appTrafTotalsResetted,

    true, // Rpc_StatConnManager_deleteConn,
    0, // Rpc_StatConnManager_connChanged,

    true, // Rpc_TaskManager_runTask,
    true, // Rpc_TaskManager_abortTask,
    0, // Rpc_TaskManager_taskStarted,
    0, // Rpc_TaskManager_taskFinished,
    0, // Rpc_TaskManager_appVersionUpdated,
    0, // Rpc_TaskManager_appVersionDownloaded,
    0, // Rpc_TaskManager_zonesDownloaded,
};

bool commandRequiresValidation(Command cmd)
{
    return g_commandValidations[cmd];
}

QDebug operator<<(QDebug debug, Command cmd)
{
    debug << commandString(cmd);
    return debug;
}

QDebug operator<<(QDebug debug, RpcManager rpcManager)
{
    debug << rpcManagerString(rpcManager);
    return debug;
}

}
