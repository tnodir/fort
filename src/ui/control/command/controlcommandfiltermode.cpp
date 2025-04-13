#include "controlcommandfiltermode.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/ioc/ioccontainer.h>

namespace {

QStringList filterModeNames()
{
    // Sync with enum FilterMode
    return { "learn", "ask", "block", "allow", "ignore" };
}

FirewallConf::FilterMode filterModeByText(const QString &commandText, bool &report)
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

    if (commandText == "report") {
        report = true;
    }

    return FirewallConf::ModeInvalid;
}

bool reportCommandFilterMode(ProcessCommandResult &r, FirewallConf *conf)
{
    const auto filterMode = conf->filterMode();

    r.commandResult = Control::CommandResult(Control::CommandResultBase + filterMode);

    r.errorMessage = filterModeNames().value(filterMode);

    return true;
}

bool processCommandFilterModeAction(
        ProcessCommandResult &r, FirewallConf::FilterMode filterMode, bool report)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();

    if (report) {
        return reportCommandFilterMode(r, conf);
    }

    conf->setFilterMode(filterMode);

    return confManager->saveFlags();
}

}

bool ControlCommandFilterMode::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    bool report = false;
    const FirewallConf::FilterMode filterMode =
            filterModeByText(p.args.value(0).toString(), report);

    if (filterMode == FirewallConf::ModeInvalid && !report) {
        r.errorMessage = "Usage: filter-mode learn|ask|block|allow|ignore|report";
        return false;
    }

    if (!checkCommandActionPassword(r, filterMode))
        return false;

    return processCommandFilterModeAction(r, filterMode, report);
}
