#include "controlcommandblock.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/ioc/ioccontainer.h>

namespace {

bool processCommandBlockAction(FirewallConf::BlockTrafficType blockAction)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();
    conf->setBlockTrafficIndex(blockAction);

    return confManager->saveFlags();
}

FirewallConf::BlockTrafficType blockActionByText(const QString &commandText)
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

    return FirewallConf::BlockTrafficInvalid;
}

}

bool ControlCommandBlock::processCommand(const ProcessCommandArgs &p)
{
    const FirewallConf::BlockTrafficType blockAction =
            blockActionByText(p.args.value(0).toString());
    if (blockAction == FirewallConf::BlockTrafficInvalid) {
        p.errorMessage = "Usage: block no|inet|lan|inet-lan|all";
        return false;
    }

    if (!checkCommandActionPassword(p, blockAction))
        return false;

    return processCommandBlockAction(blockAction);
}
