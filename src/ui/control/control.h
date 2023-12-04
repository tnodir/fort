#ifndef CONTROL_H
#define CONTROL_H

#include <QDebug>
#include <QObject>

namespace Control {

enum Command : qint8 {
    CommandNone = 0,

    Prog,

    Rpc_Result_Ok,
    Rpc_Result_Error,

    Rpc_RpcManager_initClient,

    Rpc_AppInfoManager_lookupAppInfo,
    Rpc_AppInfoManager_checkLookupInfoFinished,

    Rpc_ConfManager_saveVariant,
    Rpc_ConfManager_addApp,
    Rpc_ConfManager_deleteApps,
    Rpc_ConfManager_purgeApps,
    Rpc_ConfManager_updateApp,
    Rpc_ConfManager_updateAppsBlocked,
    Rpc_ConfManager_updateAppName,
    Rpc_ConfManager_addZone,
    Rpc_ConfManager_deleteZone,
    Rpc_ConfManager_updateZone,
    Rpc_ConfManager_updateZoneName,
    Rpc_ConfManager_updateZoneEnabled,
    Rpc_ConfManager_checkPassword,
    Rpc_ConfManager_confChanged,
    Rpc_ConfManager_appEndTimesUpdated,
    Rpc_ConfManager_appAlerted,
    Rpc_ConfManager_appChanged,
    Rpc_ConfManager_appUpdated,
    Rpc_ConfManager_zoneAdded,
    Rpc_ConfManager_zoneRemoved,
    Rpc_ConfManager_zoneUpdated,

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
    Rpc_ConfManager,
    Rpc_DriverManager,
    Rpc_QuotaManager,
    Rpc_StatManager,
    Rpc_StatBlockManager,
    Rpc_ServiceInfoManager,
    Rpc_TaskManager,
};

RpcManager managerByCommand(Command cmd);

bool commandRequiresValidation(Command cmd);

QDebug operator<<(QDebug debug, Command cmd);
QDebug operator<<(QDebug debug, RpcManager rpcManager);

}

#endif // CONTROL_H
