#include "controlcommandblock.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/ioc/ioccontainer.h>

namespace {

QStringList blockTrafficNames()
{
    // Sync with enum FirewallConf::BlockTrafficType
    return { "no", "inet", "lan", "inet-lan", "all" };
}

FirewallConf::BlockTrafficType blockActionByText(const QString &commandText, bool &report)
{
    if (commandText == "no")
        return FirewallConf::BlockTrafficNone;

    if (commandText == "inet")
        return FirewallConf::BlockTrafficInet;

    if (commandText == "lan")
        return FirewallConf::BlockTrafficLan;

    if (commandText == "inet-lan")
        return FirewallConf::BlockTrafficInetLan;

    if (commandText == "all")
        return FirewallConf::BlockTrafficAll;

    if (commandText == "report") {
        report = true;
    }

    return FirewallConf::BlockTrafficInvalid;
}

bool reportCommandBlock(ProcessCommandResult &r, FirewallConf *conf)
{
    const int blockTrafficIndex = conf->blockTrafficIndex();

    r.commandResult = Control::CommandResult(Control::CommandResultBase + blockTrafficIndex);

    r.errorMessage = blockTrafficNames().value(blockTrafficIndex);

    return true;
}

bool processCommandBlockAction(
        ProcessCommandResult &r, FirewallConf::BlockTrafficType blockAction, bool report)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();

    if (report) {
        return reportCommandBlock(r, conf);
    }

    conf->setBlockTrafficIndex(blockAction);

    return confManager->saveFlags();
}

}

bool ControlCommandBlock::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    bool report = false;
    const FirewallConf::BlockTrafficType blockAction =
            blockActionByText(p.args.value(0).toString(), report);

    if (blockAction == FirewallConf::BlockTrafficInvalid && !report) {
        r.errorMessage = "Usage: block no|inet|lan|inet-lan|all|report";
        return false;
    }

    if (!checkCommandActionPassword(r, blockAction))
        return false;

    return processCommandBlockAction(r, blockAction, report);
}
