#include "control.h"

namespace Control {

#define CASE_STRING(id)                                                                            \
    case (id):                                                                                     \
        return #id;

const char *const commandString(Command cmd)
{
    switch (cmd) {
        CASE_STRING(CommandNone)

        CASE_STRING(Prog)

        CASE_STRING(Rpc_Result_Ok)
        CASE_STRING(Rpc_Result_Error)
        CASE_STRING(Rpc_RpcManager_initClient)

        CASE_STRING(Rpc_AppInfoManager_lookupAppInfo)
        CASE_STRING(Rpc_AppInfoManager_checkLookupFinished)

        CASE_STRING(Rpc_ConfManager_save)
        CASE_STRING(Rpc_ConfManager_addApp)
        CASE_STRING(Rpc_ConfManager_deleteApp)
        CASE_STRING(Rpc_ConfManager_updateApp)
        CASE_STRING(Rpc_ConfManager_updateAppName)
        CASE_STRING(Rpc_ConfManager_addZone)
        CASE_STRING(Rpc_ConfManager_deleteZone)
        CASE_STRING(Rpc_ConfManager_updateZone)
        CASE_STRING(Rpc_ConfManager_updateZoneName)
        CASE_STRING(Rpc_ConfManager_updateZoneEnabled)
        CASE_STRING(Rpc_ConfManager_checkPassword)
        CASE_STRING(Rpc_ConfManager_confChanged)
        CASE_STRING(Rpc_ConfManager_appEndTimesUpdated)
        CASE_STRING(Rpc_ConfManager_appAdded)
        CASE_STRING(Rpc_ConfManager_appRemoved)
        CASE_STRING(Rpc_ConfManager_appUpdated)
        CASE_STRING(Rpc_ConfManager_zoneAdded)
        CASE_STRING(Rpc_ConfManager_zoneRemoved)
        CASE_STRING(Rpc_ConfManager_zoneUpdated)

        CASE_STRING(Rpc_DriverManager_updateState)

        CASE_STRING(Rpc_QuotaManager_alert)

        CASE_STRING(Rpc_StatManager_deleteStatApp)
        CASE_STRING(Rpc_StatManager_deleteConn)
        CASE_STRING(Rpc_StatManager_deleteConnAll)
        CASE_STRING(Rpc_StatManager_resetAppTrafTotals)
        CASE_STRING(Rpc_StatManager_clearTraffic)
        CASE_STRING(Rpc_StatManager_trafficCleared)
        CASE_STRING(Rpc_StatManager_appStatRemoved)
        CASE_STRING(Rpc_StatManager_appCreated)
        CASE_STRING(Rpc_StatManager_trafficAdded)
        CASE_STRING(Rpc_StatManager_connBlockAdded)
        CASE_STRING(Rpc_StatManager_connRemoved)
        CASE_STRING(Rpc_StatManager_appTrafTotalsResetted)

        CASE_STRING(Rpc_TaskManager_runTask)
        CASE_STRING(Rpc_TaskManager_abortTask)
        CASE_STRING(Rpc_TaskManager_taskStarted)
        CASE_STRING(Rpc_TaskManager_taskFinished)

    default:
        return nullptr;
    }
}

const char *const rpcManagerString(RpcManager rpcManager)
{
    switch (rpcManager) {
        CASE_STRING(Rpc_NoneManager)
        CASE_STRING(Rpc_AppInfoManager)
        CASE_STRING(Rpc_ConfManager)
        CASE_STRING(Rpc_DriverManager)
        CASE_STRING(Rpc_QuotaManager)
        CASE_STRING(Rpc_StatManager)
        CASE_STRING(Rpc_TaskManager)
    default:
        return nullptr;
    }
}

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
    case Rpc_ConfManager_checkPassword:
    case Rpc_ConfManager_confChanged:
    case Rpc_ConfManager_appEndTimesUpdated:
    case Rpc_ConfManager_appAdded:
    case Rpc_ConfManager_appRemoved:
    case Rpc_ConfManager_appUpdated:
    case Rpc_ConfManager_zoneAdded:
    case Rpc_ConfManager_zoneRemoved:
    case Rpc_ConfManager_zoneUpdated:
        return Rpc_ConfManager;

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

QDebug operator<<(QDebug debug, Command cmd)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    debug << commandString(cmd);
    return debug;
}

QDebug operator<<(QDebug debug, RpcManager rpcManager)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    debug << rpcManagerString(rpcManager);
    return debug;
}

}
