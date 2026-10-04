#include "group.h"

bool Group::isFlagsEqual(const Group &o) const
{
    return enabled == o.enabled && exclusive == o.exclusive && periodEnabled == o.periodEnabled;
}

bool Group::isOptionsEqual(const Group &o) const
{
    return isFlagsEqual(o) && periodId == o.periodId && ruleId == o.ruleId && notes == o.notes;
}

bool Group::isNameEqual(const Group &o) const
{
    return groupName == o.groupName;
}

QString Group::menuLabel(const QString &periodName) const
{
    if (periodName.isEmpty())
        return groupName;

    return groupName + " (" + periodName + ')';
}
