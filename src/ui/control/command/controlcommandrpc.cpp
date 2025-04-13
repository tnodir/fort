#include "controlcommandrpc.h"

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

bool ControlCommandRpc::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    return IoC<RpcManager>()->processCommandRpc(p, r);
}
