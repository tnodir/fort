#include "serviceinfomanagerrpc.h"

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

ServiceInfoManagerRpc::ServiceInfoManagerRpc(QObject *parent) : ServiceInfoManager(parent) { }

void ServiceInfoManagerRpc::trackService(const QString &serviceName)
{
    IoC<RpcManager>()->invokeOnServer(
            Control::Rpc_ServiceInfoManager_trackService, { serviceName });
}

void ServiceInfoManagerRpc::revertService(const QString &serviceName)
{
    IoC<RpcManager>()->invokeOnServer(
            Control::Rpc_ServiceInfoManager_revertService, { serviceName });
}
