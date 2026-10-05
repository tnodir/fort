#include "controlcommandmanager.h"

#include <control/controlworker.h>
#include <rpc/rpcmanager.h>

#include "controlcommandbackup.h"
#include "controlcommandblock.h"
#include "controlcommandconf.h"
#include "controlcommandfilter.h"
#include "controlcommandfiltermode.h"
#include "controlcommandgraph.h"
#include "controlcommandgroup.h"
#include "controlcommandhome.h"
#include "controlcommandprog.h"
#include "controlcommandrpc.h"
#include "controlcommandzone.h"

namespace {

static const processCommand_func processCommand_funcList[] = {
    &ControlCommandHome::processCommand, // Control::CommandHome,
    &ControlCommandGraph::processCommand, // Control::CommandGraph,
    &ControlCommandFilter::processCommand, // Control::CommandFilter,
    &ControlCommandFilterMode::processCommand, // Control::CommandFilterMode,
    &ControlCommandBlock::processCommand, // Control::CommandBlock,
    &ControlCommandProg::processCommand, // Control::CommandProg,
    &ControlCommandGroup::processCommand, // Control::CommandGroup,
    &ControlCommandConf::processCommand, // Control::CommandConf,
    &ControlCommandBackup::processCommand, // Control::CommandBackup,
    &ControlCommandZone::processCommand, // Control::CommandZone,
};

}

bool ControlCommandManager::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processCommand_func func = RpcManager::getProcessFunc(p.command, processCommand_funcList,
            Control::CommandHome, Control::CommandZone, &ControlCommandRpc::processCommand);

    const bool ok = func(p, r);

    if (!ok && r.errorMessage.isEmpty()) {
        r.errorMessage = "Invalid command";
    }

    if (func != &ControlCommandRpc::processCommand) {
        r.ok = ok;
        r.isSendResult = true;

        r.args = { r.commandResult, r.errorMessage };
    }

    if (r.isSendResult) {
        p.worker->sendResult(p.requestId, r.ok, r.args);
    }

    return ok;
}
