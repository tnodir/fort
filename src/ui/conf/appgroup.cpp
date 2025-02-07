#include "appgroup.h"

#include <util/dateutil.h>
#include <util/formatutil.h>

bool AppGroup::isNameEqual(const AppGroup &o) const
{
    return groupName == o.groupName;
}

bool AppGroup::isOptionsEqual(const AppGroup &o) const
{
    return enabled == o.enabled && periodEnabled == o.periodEnabled && notes == o.notes
            && periodFrom == o.periodFrom && periodTo == o.periodTo;
}

QString AppGroup::menuLabel() const
{
    QString text = groupName;

    if (periodEnabled) {
        text += QLatin1Char(' ') + DateUtil::formatPeriod(periodFrom, periodTo);
    }

    return text;
}

void AppGroupPeriod::setupPeriodTimes(const QString &periodFrom, const QString &periodTo)
{
    periodFromTime = DateUtil::parseTime(periodFrom);
    periodToTime = DateUtil::parseTime(periodTo);
}

bool AppGroupPeriod::isTimeInPeriod(QTime time) const
{
    return DateUtil::isTimeInPeriod(time, periodFromTime, periodToTime);
}
