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

FilterAction filterActionByText(const QString &commandText)
{
    if (commandText == "on")
        return FilterActionOn;

    if (commandText == "off")
        return FilterActionOff;

    return FilterActionInvalid;
}

bool processCommandFilterAction(FilterAction filterAction)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();
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

    return processCommandFilterAction(filterAction);
}
