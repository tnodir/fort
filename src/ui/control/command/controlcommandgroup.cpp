#include "controlcommandgroup.h"

#include <conf/confgroupmanager.h>
#include <conf/group.h>
#include <fortglobal.h>

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

bool reportCommandGroupAction(ProcessCommandResult &r, const Group &group)
{
    const auto groupAction = group.enabled ? GroupActionOn : GroupActionOff;

    r.commandResult = Control::CommandResult(Control::CommandResultBase + groupAction);

    r.errorMessage = groupActionNames().value(groupAction);

    return true;
}

bool processCommandGroupAction(
        ProcessCommandResult &r, int groupId, GroupAction groupAction, bool report)
{
    auto confGroupManager = Fort::confGroupManager();

    Group group;

    if (!confGroupManager->loadGroupById(group, groupId)) {
        r.commandResult = Control::CommandResultError;
        r.errorMessage = "Group not found";
        return true;
    }

    if (report) {
        return reportCommandGroupAction(r, group);
    }

    return confGroupManager->updateGroupEnabled(group.groupId, groupAction == GroupActionOn);
}

}

bool ControlCommandGroup::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    bool report = false;
    const GroupAction groupAction = groupActionByText(p.args.value(0).toString(), report);

    const bool isValidAction = (groupAction != GroupActionInvalid || report);
    if (!isValidAction || p.args.size() < 2) {
        r.errorMessage = "Usage: group on|off|report [group-id]";
        return false;
    }

    if (!checkCommandActionPassword(r, groupAction))
        return false;

    const int groupId = p.args.value(1).toInt();

    const bool ok = processCommandGroupAction(r, groupId, groupAction, report);

    uncheckCommandActionPassword();

    return ok;
}
