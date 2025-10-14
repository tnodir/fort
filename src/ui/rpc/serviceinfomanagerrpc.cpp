#include "serviceinfomanagerrpc.h"

#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

namespace {

inline bool serviceInfoManager_trackService(
        ServiceInfoManager *serviceInfoManager, const ProcessCommandArgs &p)
{
    serviceInfoManager->trackService(p.args.value(0).toString());
    return true;
}

inline bool serviceInfoManager_revertService(
        ServiceInfoManager *serviceInfoManager, const ProcessCommandArgs &p)
{
    serviceInfoManager->revertService(p.args.value(0).toString());
    return true;
}

}

ServiceInfoManagerRpc::ServiceInfoManagerRpc(QObject *parent) : ServiceInfoManager(parent) { }

void ServiceInfoManagerRpc::trackService(const QString &serviceName)
{
    rpcManager()->invokeOnServer(Control::Rpc_ServiceInfoManager_trackService, { serviceName });
}

void ServiceInfoManagerRpc::revertService(const QString &serviceName)
{
    rpcManager()->invokeOnServer(Control::Rpc_ServiceInfoManager_revertService, { serviceName });
}

bool ServiceInfoManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    auto serviceInfoManager = Fort::serviceInfoManager();

    switch (p.command) {
    case Control::Rpc_ServiceInfoManager_trackService: {
        return serviceInfoManager_trackService(serviceInfoManager, p);
    }
    case Control::Rpc_ServiceInfoManager_revertService: {
        return serviceInfoManager_revertService(serviceInfoManager, p);
    }
    default:
        return false;
    }
}
