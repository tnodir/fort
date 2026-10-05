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

inline constexpr ControlCommandHome commandHome;
inline constexpr ControlCommandGraph commandGraph;
inline constexpr ControlCommandFilter commandFilter;
inline constexpr ControlCommandFilterMode commandFilterMode;
inline constexpr ControlCommandBlock commandBlock;
inline constexpr ControlCommandProg commandProg;
inline constexpr ControlCommandGroup commandGroup;
inline constexpr ControlCommandConf commandConf;
inline constexpr ControlCommandBackup commandBackup;
inline constexpr ControlCommandZone commandZone;

inline constexpr ControlCommandRpc commandRpc;

inline constexpr const ControlCommandBase *commandList[] = {
    &commandHome, // Control::CommandHome,
    &commandGraph, // Control::CommandGraph,
    &commandFilter, // Control::CommandFilter,
    &commandFilterMode, // Control::CommandFilterMode,
    &commandBlock, // Control::CommandBlock,
    &commandProg, // Control::CommandProg,
    &commandGroup, // Control::CommandGroup,
    &commandConf, // Control::CommandConf,
    &commandBackup, // Control::CommandBackup,
    &commandZone, // Control::CommandZone,
};

const ControlCommandBase *commandByType(Control::Command command)
{
    return RpcManager::getProcessFunc<const ControlCommandBase>(
            command, commandList, Control::CommandHome, Control::CommandZone, &commandRpc);
}

}

bool ControlCommandManager::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const ControlCommandBase *command = commandByType(p.command);

    const bool ok = command->processCommand(p, r);

    if (!ok && r.errorMessage.isEmpty()) {
        r.errorMessage = "Invalid command";
    }

    if (command != &commandRpc) {
        r.ok = ok;
        r.isSendResult = true;

        r.args = { r.commandResult, r.errorMessage };
    }

    if (r.isSendResult) {
        p.worker->sendResult(p.requestId, r.ok, r.args);
    }

    return ok;
}
