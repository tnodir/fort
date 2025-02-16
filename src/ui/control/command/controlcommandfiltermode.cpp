#include "controlcommandfiltermode.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/ioc/ioccontainer.h>

namespace {

FirewallConf::FilterMode filterModeByText(const QString &commandText)
{
    if (commandText == "learn")
        return FirewallConf::ModeAutoLearn;

    if (commandText == "ask")
        return FirewallConf::ModeAskToConnect;

    if (commandText == "block")
        return FirewallConf::ModeBlockAll;

    if (commandText == "allow")
        return FirewallConf::ModeAllowAll;

    if (commandText == "ignore")
        return FirewallConf::ModeIgnore;

    return FirewallConf::ModeInvalid;
}

bool processCommandFilterModeAction(FirewallConf::FilterMode filterMode)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();
    conf->setFilterMode(filterMode);

    return confManager->saveFlags();
}

}

bool ControlCommandFilterMode::processCommand(const ProcessCommandArgs &p)
{
    const FirewallConf::FilterMode filterMode = filterModeByText(p.args.value(0).toString());
    if (filterMode == FirewallConf::ModeInvalid) {
        p.errorMessage = "Usage: filter-mode learn|ask|block|allow|ignore";
        return false;
    }

    if (!checkCommandActionPassword(p, filterMode))
        return false;

    return processCommandFilterModeAction(filterMode);
}
