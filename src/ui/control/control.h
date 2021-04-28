#ifndef CONTROL_H
#define CONTROL_H

#include <QObject>

namespace Control {

enum Command : qint8 { CommandNone = 0, CommandConf, CommandProg, CommandRpc };

enum RpcObject : qint8 {
    Rpc_None = 0,
    Rpc_AppInfoManager,
    Rpc_ConfManager,
    Rpc_DriverManager,
    Rpc_QuotaManager,
    Rpc_StatManager,
    Rpc_TaskManager,
};

}

#endif // CONTROL_H
