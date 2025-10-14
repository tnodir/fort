#include "controlcommandgroup.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

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
    auto confManager = Fort::confManager();

    auto conf = confManager->conf();

    if (groupIndex < 0 || groupIndex >= conf->appGroups().size()) {
        r.commandResult = Control::CommandResultError;
        r.errorMessage = "Group not found";
        return true;
    }

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

    const bool isValidAction = (groupAction != GroupActionInvalid || report);
    if (!isValidAction || p.args.size() < 2) {
        r.errorMessage = "Usage: group on|off|report [group-index]";
        return false;
    }

    if (!checkCommandActionPassword(r, groupAction))
        return false;

    const int groupIndex = p.args.value(1).toInt();

    const bool ok = processCommandGroupAction(r, groupIndex, groupAction, report);

    uncheckCommandActionPassword();

    return ok;
}
