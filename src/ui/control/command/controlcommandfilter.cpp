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

bool reportCommandFilter(const ProcessCommandArgs &p, FirewallConf *conf)
{
    if (conf->filterEnabled()) {
        p.commandResult = Control::CommandResultFilter_On;
        p.errorMessage = "on";
    } else {
        p.commandResult = Control::CommandResultFilter_Off;
        p.errorMessage = "off";
    }

    return true;
}

bool processCommandFilterAction(const ProcessCommandArgs &p, FilterAction filterAction)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();

    if (filterAction == FilterActionReport) {
        return reportCommandFilter(p, conf);
    }

    conf->setFilterEnabled(filterAction == FilterActionOn);

    return confManager->saveFlags();
}

}

bool ControlCommandFilter::processCommand(const ProcessCommandArgs &p)
{
    const FilterAction filterAction = filterActionByText(p.args.value(0).toString());
    if (filterAction == FilterActionInvalid) {
        p.errorMessage = "Usage: filter on|off";
        return false;
    }

    if (!checkCommandActionPassword(p, filterAction))
        return false;

    return processCommandFilterAction(p, filterAction);
}
