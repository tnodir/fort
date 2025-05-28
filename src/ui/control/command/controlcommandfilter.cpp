#include "controlcommandfilter.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/ioc/ioccontainer.h>

namespace {

enum FilterAction : qint8 {
    FilterActionInvalid = -1,
    FilterActionOn = 0,
    FilterActionOff,
};

FilterAction filterActionByText(const QString &commandText, bool &report)
{
    if (commandText == "on")
        return FilterActionOn;

    if (commandText == "off")
        return FilterActionOff;

    if (commandText == "report") {
        report = true;
    }

    return FilterActionInvalid;
}

bool reportCommandFilter(ProcessCommandResult &r, FirewallConf *conf)
{
    const bool filterEnabled = conf->filterEnabled();

    const FilterAction filterAction = filterEnabled ? FilterActionOn : FilterActionOff;

    r.commandResult = Control::CommandResult(Control::CommandResultBase + filterAction);

    r.errorMessage = filterEnabled ? "on" : "off";

    return true;
}

bool processCommandFilterAction(ProcessCommandResult &r, FilterAction filterAction, bool report)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();

    if (report) {
        return reportCommandFilter(r, conf);
    }

    conf->setFilterEnabled(filterAction == FilterActionOn);

    return confManager->saveFlags();
}

}

bool ControlCommandFilter::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    bool report = false;
    const FilterAction filterAction = filterActionByText(p.args.value(0).toString(), report);

    if (filterAction == FilterActionInvalid && !report) {
        r.errorMessage = "Usage: filter on|off|report";
        return false;
    }

    if (!checkCommandActionPassword(r, filterAction))
        return false;

    const bool ok = processCommandFilterAction(r, filterAction, report);

    uncheckCommandActionPassword();

    return ok;
}
