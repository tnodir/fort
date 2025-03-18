#include "controlcommandmanager.h"

#include <rpc/rpcmanager.h>

#include "controlcommandbackup.h"
#include "controlcommandblock.h"
#include "controlcommandconf.h"
#include "controlcommandfilter.h"
#include "controlcommandfiltermode.h"
#include "controlcommandhome.h"
#include "controlcommandprog.h"
#include "controlcommandrpc.h"
#include "controlcommandzone.h"

namespace {

using processCommand_func = bool (*)(const ProcessCommandArgs &p);

static const processCommand_func processCommand_funcList[] = {
    &ControlCommandHome::processCommand, // Control::CommandHome,
    &ControlCommandFilter::processCommand, // Control::CommandFilter,
    &ControlCommandFilterMode::processCommand, // Control::CommandFilterMode,
    &ControlCommandBlock::processCommand, // Control::CommandBlock,
    &ControlCommandProg::processCommand, // Control::CommandProg,
    &ControlCommandConf::processCommand, // Control::CommandConf,
    &ControlCommandBackup::processCommand, // Control::CommandBackup,
    &ControlCommandZone::processCommand, // Control::CommandZone,
};

}

bool ControlCommandManager::processCommand(const ProcessCommandArgs &p)
{
    const processCommand_func func = RpcManager::getProcessFunc(p.command, processCommand_funcList,
            Control::CommandHome, Control::CommandZone, &ControlCommandRpc::processCommand);

    const bool ok = func(p);

    if (!ok && p.errorMessage.isEmpty()) {
        p.errorMessage = "Invalid command";
    }

    return ok;
}
