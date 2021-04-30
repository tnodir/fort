#ifndef CONTROL_H
#define CONTROL_H

#include <QObject>

namespace Control {

enum Command : qint8 {
    CommandNone = 0,
    Conf,
    Prog,
    Rpc_RpcManager_initClient,
    Rpc_AppInfoManager_lookupAppInfo,
    Rpc_AppInfoManager_checkLookupFinished,
    Rpc_ConfManager_,
    Rpc_DriverManager_,
    Rpc_QuotaManager_alert,
    Rpc_StatManager_,
    Rpc_TaskManager_,
};

}

#endif // CONTROL_H
