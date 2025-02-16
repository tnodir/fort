#include "controlcommandrpc.h"

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

bool ControlCommandRpc::processCommand(const ProcessCommandArgs &p)
{
    return IoC<RpcManager>()->processCommandRpc(p);
}
