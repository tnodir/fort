#include "controlcommandmanager.h"

#include <control/controlmanager.h>
#include <control/controlworker.h>
#include <fortglobal.h>
#include <rpc/rpcmanager.h>

#include "controlcommandaddr.h"
#include "controlcommandbackup.h"
#include "controlcommandblock.h"
#include "controlcommandconf.h"
#include "controlcommandconn.h"
#include "controlcommandfilter.h"
#include "controlcommandfiltermode.h"
#include "controlcommandgraph.h"
#include "controlcommandgroup.h"
#include "controlcommandhome.h"
#include "controlcommandpending.h"
#include "controlcommandperiod.h"
#include "controlcommandprog.h"
#include "controlcommandrpc.h"
#include "controlcommandrule.h"
#include "controlcommandsimulate.h"
#include "controlcommandtraffic.h"
#include "controlcommandutil.h"
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
inline constexpr ControlCommandRule commandRule;
inline constexpr ControlCommandAddr commandAddr;
inline constexpr ControlCommandPeriod commandPeriod;
inline constexpr ControlCommandSimulate commandSimulate;
inline constexpr ControlCommandConn commandConn;
inline constexpr ControlCommandTraffic commandTraffic;
inline constexpr ControlCommandPending commandPending;

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
    &commandRule, // Control::CommandRule,
    &commandAddr, // Control::CommandAddr,
    &commandPeriod, // Control::CommandPeriod,
    &commandSimulate, // Control::CommandSimulate,
    &commandConn, // Control::CommandConn,
    &commandTraffic, // Control::CommandTraffic,
    &commandPending, // Control::CommandPending,
};

const ControlCommandBase *commandByType(Control::Command command)
{
    return RpcManager::getProcessFunc<const ControlCommandBase>(
            command, commandList, Control::CommandHome, Control::CommandPending, &commandRpc);
}

}

bool ControlCommandManager::processCommandClient(
        Control::Command command, QVariantList &args, ProcessCommandResult &r)
{
    const ControlCommandBase *controlCommand = commandByType(command);

    return controlCommand->prepareClientArgs(args, r)
            && Fort::controlManager()->postCommand(command, args, &r)
            && controlCommand->processClientResult(args, r);
}

bool ControlCommandManager::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const ControlCommandBase *command = commandByType(p.command);

    const bool ok = command->processCommand(p, r);

    if (command != &commandRpc) {
        ControlCommandUtil::uncheckCommandActionPassword();
    }

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
