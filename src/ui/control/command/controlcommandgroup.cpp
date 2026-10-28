#include "controlcommandgroup.h"

#include <conf/confgroupmanager.h>
#include <conf/confrulemanager.h>
#include <conf/group.h>
#include <fortglobal.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum GroupAction : qint8 {
    GroupActionInvalid = -1,
    GroupActionOn = 0,
    GroupActionOff,
    GroupActionAdd,
    GroupActionDel,
    GroupActionSet,
};

QStringList groupActionNames()
{
    // Sync with enum GroupAction
    return { "on", "off", "add", "del", "set" };
}

GroupAction groupActionByText(const QString &commandText, bool &report)
{
    report = (commandText == "report");

    return GroupAction(groupActionNames().indexOf(commandText));
}

int groupActionArgsCount(GroupAction groupAction)
{
    // The set action has the fields' arguments
    return (groupAction == GroupActionSet) ? 3 : 2;
}

bool reportCommandGroupAction(ProcessCommandResult &r, const Group &group)
{
    const auto groupAction = group.enabled ? GroupActionOn : GroupActionOff;

    r.commandResult = Control::CommandResult(Control::CommandResultBase + groupAction);

    r.errorMessage = groupActionNames().value(groupAction);

    return true;
}

QVariantMap groupFields(const Group &group)
{
    return {
        { "enabled", group.enabled },
        { "exclusive", group.exclusive },
        { "periodEnabled", group.periodEnabled },
        { "period", QVariant::fromValue(group.periodId) },
        { "rule", confRuleManager()->ruleNameById(group.ruleId) },
        { "name", group.groupName },
        { "notes", group.notes },
    };
}

void fillGroupOptions(Group &group, const QVariantMap &fields)
{
    group.enabled = fields["enabled"].toBool();
    group.exclusive = fields["exclusive"].toBool();
    group.periodEnabled = fields["periodEnabled"].toBool();
    group.periodId = fields["period"].toUInt();
    group.notes = fields["notes"].toString();
}

bool fillGroupName(ProcessCommandResult &r, Group &group, const QString &groupName)
{
    if (groupName.isEmpty()) {
        r.errorMessage = ControlCommandUtil::invalidValueText("name", groupName);
        return false;
    }

    group.groupName = groupName;

    return true;
}

bool setGroupFields(ProcessCommandResult &r, Group &group, const QVariantList &args, int index)
{
    QVariantMap fields = groupFields(group);

    if (!ControlCommandUtil::parseFieldArgs(args, index, fields, r.errorMessage))
        return false;

    if (!fillGroupName(r, group, fields["name"].toString()))
        return false;

    if (!ControlCommandUtil::findRuleIdByName(
                fields["rule"].toString(), group.ruleId, r.errorMessage))
        return false;

    fillGroupOptions(group, fields);

    return confGroupManager()->addOrUpdateGroup(group);
}

bool addGroup(ProcessCommandResult &r, const QVariantList &args)
{
    Group group;
    group.groupName = args.value(1).toString();

    if (!setGroupFields(r, group, args, 2))
        return false;

    r.errorMessage = QString::number(group.groupId);

    return true;
}

bool processCommandGroupAction(
        ProcessCommandResult &r, const QVariantList &args, GroupAction groupAction, bool report)
{
    if (groupAction == GroupActionAdd) {
        return addGroup(r, args);
    }

    auto confGroupManager = Fort::confGroupManager();

    Group group;

    if (!confGroupManager->loadGroupById(group, args.value(1).toInt())) {
        ControlCommandUtil::setErrorResult(r, "Group not found");
        return true;
    }

    if (report) {
        return reportCommandGroupAction(r, group);
    }

    switch (groupAction) {
    case GroupActionDel: {
        return confGroupManager->deleteGroup(group.groupId);
    }
    case GroupActionSet: {
        return setGroupFields(r, group, args, 2);
    }
    default: // On or Off
        return confGroupManager->updateGroupEnabled(group.groupId, groupAction == GroupActionOn);
    }
}

}

bool ControlCommandGroup::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    bool report = false;
    const GroupAction groupAction = groupActionByText(p.args.value(0).toString(), report);

    const bool isValidAction = (groupAction != GroupActionInvalid || report);
    if (!isValidAction || p.args.size() < groupActionArgsCount(groupAction)) {
        r.errorMessage = "Usage: group on|off|report|del <group-id>"
                         " | group add <group-name> [<key>=<value>...]"
                         " | group set <group-id> <key>=<value>...";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, groupAction))
        return false;

    return processCommandGroupAction(r, p.args, groupAction, report);
}
