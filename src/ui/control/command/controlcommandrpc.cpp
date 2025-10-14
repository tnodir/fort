#include "controlcommandrpc.h"

#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

bool ControlCommandRpc::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    return rpcManager()->processCommandRpc(p, r);
}
