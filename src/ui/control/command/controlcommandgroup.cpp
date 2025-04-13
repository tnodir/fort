#include "controlcommandgroup.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

enum GroupAction : qint8 {
    GroupActionInvalid = -1,
    GroupActionOn = 0,
    GroupActionOff,
};

QStringList groupActionNames()
{
    // Sync with enum GroupAction
    return { "on", "off" };
}

GroupAction groupActionByText(const QString &commandText, bool &report)
{
    if (commandText == "on")
        return GroupActionOn;

    if (commandText == "off")
        return GroupActionOff;

    if (commandText == "report") {
        report = true;
    }

    return GroupActionInvalid;
}

bool reportCommandGroupAction(ProcessCommandResult &r, FirewallConf *conf, int groupIndex)
{
    const auto groupAction = conf->appGroupEnabled(groupIndex) ? GroupActionOn : GroupActionOff;

    r.commandResult = Control::CommandResult(Control::CommandResultBase + groupAction);

    r.errorMessage = groupActionNames().value(groupAction);

    return true;
}

bool processCommandGroupAction(
        ProcessCommandResult &r, int groupIndex, GroupAction groupAction, bool report)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();

    if (report) {
        return reportCommandGroupAction(r, conf, groupIndex);
    }

    conf->setAppGroupEnabled(groupIndex, groupAction == GroupActionOn);

    return confManager->saveFlags();
}

}

bool ControlCommandGroup::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    bool report = false;
    const GroupAction groupAction = groupActionByText(p.args.value(0).toString(), report);

    if (groupAction == GroupActionInvalid && !report) {
        r.errorMessage = "Usage: group on|off|report [group-index]";
        return false;
    }

    if (!checkCommandActionPassword(r, groupAction))
        return false;

    const int groupIndex = p.args.value(1).toInt();

    return processCommandGroupAction(r, groupIndex, groupAction, report);
}
