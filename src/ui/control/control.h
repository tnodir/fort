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
    Rpc_ConfManager_onConfChanged,
    Rpc_ConfManager_onAppEndTimesUpdated,
    Rpc_ConfManager_onAppAdded,
    Rpc_ConfManager_onAppRemoved,
    Rpc_ConfManager_onAppUpdated,
    Rpc_ConfManager_onZoneAdded,
    Rpc_ConfManager_onZoneRemoved,
    Rpc_ConfManager_onZoneUpdated,

    Rpc_DriverManager_reinstallDriver,
    Rpc_DriverManager_uninstallDriver,
    Rpc_DriverManager_updateState,

    Rpc_QuotaManager_alert,

    Rpc_StatManager_clear,
    Rpc_StatManager_cleared,
    Rpc_StatManager_appCreated,
    Rpc_StatManager_trafficAdded,

    Rpc_TaskManager_runTask,
    Rpc_TaskManager_abortTask,
    Rpc_TaskManager_taskStarted,
    Rpc_TaskManager_taskFinished,
};

}

#endif // CONTROL_H
