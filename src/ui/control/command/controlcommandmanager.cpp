#include "controlcommandmanager.h"

#include <control/controlmanager.h>
#include <control/controlworker.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <rpc/rpcmanager.h>

#include "controlcommandaddr.h"
#include "controlcommandbackup.h"
#include "controlcommandblock.h"
#include "controlcommandconf.h"
#include "controlcommandconn.h"
#include "controlcommanddns.h"
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
#include "controlcommandwindow.h"
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
inline constexpr ControlCommandDns commandDns;
inline constexpr ControlCommandPeriod commandPeriod;
inline constexpr ControlCommandSimulate commandSimulate;
inline constexpr ControlCommandConn commandConn;
inline constexpr ControlCommandTraffic commandTraffic;
inline constexpr ControlCommandPending commandPending;
inline constexpr ControlCommandWindow commandWindow;

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
    &commandDns, // Control::CommandDns,
    &commandPeriod, // Control::CommandPeriod,
    &commandSimulate, // Control::CommandSimulate,
    &commandConn, // Control::CommandConn,
    &commandTraffic, // Control::CommandTraffic,
    &commandPending, // Control::CommandPending,
    &commandWindow, // Control::CommandWindow,
};

const ControlCommandBase *commandByType(Control::Command command)
{
    return RpcManager::getProcessFunc<const ControlCommandBase>(
            command, commandList, Control::CommandHome, Control::CommandWindow, &commandRpc);
}

bool checkCommandWindows(
        const ControlCommandBase *command, const QVariantList &args, ProcessCommandResult &r)
{
    if (!command->needsWindows(args) || !Fort::settings()->isService())
        return true;

    r.errorMessage = "The service has no windows: run the tray process";
    return false;
}

}

bool ControlCommandManager::processCommandClient(
        Control::Command command, QVariantList &args, ProcessCommandResult &r)
{
    const ControlCommandBase *controlCommand = commandByType(command);

    return controlCommand->prepareClientArgs(args, r)
            && Fort::controlManager()->postCommand(
                    command, args, &r, controlCommand->resultWaitMsecs(args))
            && controlCommand->processClientResult(args, r);
}

bool ControlCommandManager::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const ControlCommandBase *command = commandByType(p.command);

    const bool ok = checkCommandWindows(command, p.args, r) && command->processCommand(p, r);

    if (!ok && r.errorMessage.isEmpty()) {
        r.errorMessage = "Invalid command";
    }

    if (command == &commandRpc) {
        if (r.isSendResult) {
            p.worker->sendResult(p.requestId, r.ok, r.args);
        }
        return ok;
    }

    ControlCommandUtil::uncheckCommandActionPassword();

    r.ok = ok;

    if (!r.isResultDeferred) {
        sendResult(p.worker, p.requestId, r);
    }

    return ok;
}

void ControlCommandManager::sendResult(
        ControlWorker *w, quint32 requestId, const ProcessCommandResult &r)
{
    w->sendResult(requestId, r.ok, { r.commandResult, r.errorMessage });
}
