#include "controlcommandfilter.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/ioc/ioccontainer.h>

namespace {

enum FilterAction : qint8 {
    FilterActionInvalid = -1,
    FilterActionOn = 0,
    FilterActionOff,
    FilterActionReport,
};

FilterAction filterActionByText(const QString &commandText)
{
    if (commandText == "on")
        return FilterActionOn;

    if (commandText == "off")
        return FilterActionOff;

    if (commandText == "report")
        return FilterActionReport;

    return FilterActionInvalid;
}

bool reportCommandFilter(ProcessCommandResult &r, FirewallConf *conf)
{
    if (conf->filterEnabled()) {
        r.commandResult = Control::CommandResultFilter_On;
        r.errorMessage = "on";
    } else {
        r.commandResult = Control::CommandResultFilter_Off;
        r.errorMessage = "off";
    }

    return true;
}

bool processCommandFilterAction(ProcessCommandResult &r, FilterAction filterAction)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();

    if (filterAction == FilterActionReport) {
        return reportCommandFilter(r, conf);
    }

    conf->setFilterEnabled(filterAction == FilterActionOn);

    return confManager->saveFlags();
}

}

bool ControlCommandFilter::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const FilterAction filterAction = filterActionByText(p.args.value(0).toString());
    if (filterAction == FilterActionInvalid) {
        r.errorMessage = "Usage: filter on|off|report";
        return false;
    }

    if (!checkCommandActionPassword(r, filterAction))
        return false;

    return processCommandFilterAction(r, filterAction);
}
