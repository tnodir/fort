#include "control.h"

namespace Control {

RpcManager managerByCommand(Command cmd)
{
    switch (cmd) {
    case Rpc_AppInfoManager_lookupAppInfo:
    case Rpc_AppInfoManager_checkLookupFinished:
        return Rpc_AppInfoManager;

    case Rpc_ConfManager_save:
    case Rpc_ConfManager_addApp:
    case Rpc_ConfManager_deleteApp:
    case Rpc_ConfManager_updateApp:
    case Rpc_ConfManager_updateAppName:
    case Rpc_ConfManager_addZone:
    case Rpc_ConfManager_deleteZone:
    case Rpc_ConfManager_updateZone:
    case Rpc_ConfManager_updateZoneName:
    case Rpc_ConfManager_updateZoneEnabled:
    case Rpc_ConfManager_confChanged:
    case Rpc_ConfManager_appEndTimesUpdated:
    case Rpc_ConfManager_appAdded:
    case Rpc_ConfManager_appRemoved:
    case Rpc_ConfManager_appUpdated:
    case Rpc_ConfManager_zoneAdded:
    case Rpc_ConfManager_zoneRemoved:
    case Rpc_ConfManager_zoneUpdated:
        return Rpc_ConfManager;

    case Rpc_DriverManager_reinstallDriver:
    case Rpc_DriverManager_uninstallDriver:
    case Rpc_DriverManager_updateState:
        return Rpc_DriverManager;

    case Rpc_QuotaManager_alert:
        return Rpc_QuotaManager;

    case Rpc_StatManager_deleteStatApp:
    case Rpc_StatManager_deleteConn:
    case Rpc_StatManager_deleteConnAll:
    case Rpc_StatManager_resetAppTrafTotals:
    case Rpc_StatManager_clearTraffic:
    case Rpc_StatManager_trafficCleared:
    case Rpc_StatManager_appStatRemoved:
    case Rpc_StatManager_appCreated:
    case Rpc_StatManager_trafficAdded:
    case Rpc_StatManager_connBlockAdded:
    case Rpc_StatManager_connRemoved:
    case Rpc_StatManager_appTrafTotalsResetted:
        return Rpc_StatManager;

    case Rpc_TaskManager_runTask:
    case Rpc_TaskManager_abortTask:
    case Rpc_TaskManager_taskStarted:
    case Rpc_TaskManager_taskFinished:
        return Rpc_TaskManager;

    default:
        return Rpc_NoneManager;
    }
}

bool commandRequiresValidation(Command cmd)
{
    switch (cmd) {
    case Rpc_AppInfoManager_lookupAppInfo:
        return true;

    case Rpc_ConfManager_save:
    case Rpc_ConfManager_addApp:
    case Rpc_ConfManager_deleteApp:
    case Rpc_ConfManager_updateApp:
    case Rpc_ConfManager_updateAppName:
    case Rpc_ConfManager_addZone:
    case Rpc_ConfManager_deleteZone:
    case Rpc_ConfManager_updateZone:
    case Rpc_ConfManager_updateZoneName:
    case Rpc_ConfManager_updateZoneEnabled:
        return true;

    case Rpc_DriverManager_reinstallDriver:
    case Rpc_DriverManager_uninstallDriver:
        return true;

    case Rpc_StatManager_deleteStatApp:
    case Rpc_StatManager_deleteConn:
    case Rpc_StatManager_deleteConnAll:
    case Rpc_StatManager_resetAppTrafTotals:
    case Rpc_StatManager_clearTraffic:
        return true;

    case Rpc_TaskManager_runTask:
    case Rpc_TaskManager_abortTask:
        return true;

    default:
        return false;
    }
}

}
