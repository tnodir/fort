#include "group.h"

#include <util/dateutil.h>

bool Group::isFlagsEqual(const Group &o) const
{
    return enabled == o.enabled && exclusive == o.exclusive && periodEnabled == o.periodEnabled;
}

bool Group::isOptionsEqual(const Group &o) const
{
    return isFlagsEqual(o) && ruleId == o.ruleId && notes == o.notes && periodFrom == o.periodFrom
            && periodTo == o.periodTo;
}

bool Group::isNameEqual(const Group &o) const
{
    return groupName == o.groupName;
}

QString Group::menuLabel() const
{
    QString text = groupName;

    if (periodEnabled) {
        text += QLatin1Char(' ') + DateUtil::formatPeriod(periodFrom, periodTo);
    }

    return text;
}

void GroupPeriod::setupPeriodTimes(const QString &periodFrom, const QString &periodTo)
{
    periodFromTime = DateUtil::parseTime(periodFrom);
    periodToTime = DateUtil::parseTime(periodTo);
}

bool GroupPeriod::isTimeInPeriod(QTime time) const
{
    return DateUtil::isTimeInPeriod(time, periodFromTime, periodToTime);
}
