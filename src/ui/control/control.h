#ifndef CONTROL_H
#define CONTROL_H

#include <QObject>

namespace Control {

enum Command : qint8 {
    CommandNone = 0,

    Conf,
    Prog,

    Rpc_Result_Ok,
    Rpc_Result_Error,
    Rpc_RpcManager_initClient,

    Rpc_AppInfoManager_lookupAppInfo,
    Rpc_AppInfoManager_checkLookupFinished,

    Rpc_ConfManager_save,
    Rpc_ConfManager_addApp,
    Rpc_ConfManager_deleteApp,
    Rpc_ConfManager_updateApp,
    Rpc_ConfManager_updateAppName,
    Rpc_ConfManager_addZone,
    Rpc_ConfManager_deleteZone,
    Rpc_ConfManager_updateZone,
    Rpc_ConfManager_updateZoneName,
    Rpc_ConfManager_updateZoneEnabled,
    Rpc_ConfManager_confChanged,
    Rpc_ConfManager_appEndTimesUpdated,
    Rpc_ConfManager_appAdded,
    Rpc_ConfManager_appRemoved,
    Rpc_ConfManager_appUpdated,
    Rpc_ConfManager_zoneAdded,
    Rpc_ConfManager_zoneRemoved,
    Rpc_ConfManager_zoneUpdated,

    Rpc_DriverManager_reinstallDriver,
    Rpc_DriverManager_uninstallDriver,
    Rpc_DriverManager_updateState,

    Rpc_QuotaManager_alert,

    Rpc_StatManager_deleteStatApp,
    Rpc_StatManager_deleteConn,
    Rpc_StatManager_deleteConnAll,
    Rpc_StatManager_resetAppTrafTotals,
    Rpc_StatManager_clearTraffic,
    Rpc_StatManager_trafficCleared,
    Rpc_StatManager_appStatRemoved,
    Rpc_StatManager_appCreated,
    Rpc_StatManager_trafficAdded,
    Rpc_StatManager_connBlockAdded,
    Rpc_StatManager_connRemoved,
    Rpc_StatManager_appTrafTotalsResetted,

    Rpc_TaskManager_runTask,
    Rpc_TaskManager_abortTask,
    Rpc_TaskManager_taskStarted,
    Rpc_TaskManager_taskFinished,
};

}

#endif // CONTROL_H
