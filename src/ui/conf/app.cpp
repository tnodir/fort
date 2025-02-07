#include "app.h"

bool App::isFlagsEqual(const App &o) const
{
    return isBaseFlagsEqual(o) && isExtraFlagsEqual(o);
}

bool App::isBaseFlagsEqual(const App &o) const
{
    return isWildcard == o.isWildcard && applyParent == o.applyParent && applyChild == o.applyChild
            && applySpecChild == o.applySpecChild && killChild == o.killChild
            && lanOnly == o.lanOnly;
}

bool App::isExtraFlagsEqual(const App &o) const
{
    return parked == o.parked && logBlockedConn == o.logBlockedConn
            && logAllowedConn == o.logAllowedConn && blocked == o.blocked
            && killProcess == o.killProcess;
}

bool App::isZonesEqual(const App &o) const
{
    return zones.accept_mask == o.zones.accept_mask && zones.reject_mask == o.zones.reject_mask;
}

bool App::isPathsEqual(const App &o) const
{
    return appOriginPath == o.appOriginPath && appPath == o.appPath;
}

bool App::isLimitsEqual(const App &o) const
{
    return inLimitId == o.inLimitId && outLimitId == o.outLimitId;
}

bool App::isScheduleEqual(const App &o) const
{
    return scheduleAction == o.scheduleAction && scheduleTime == o.scheduleTime;
}

bool App::isOptionsEqual(const App &o) const
{
    return isFlagsEqual(o) && isZonesEqual(o) && groupId == o.groupId && ruleId == o.ruleId
            && notes == o.notes && isPathsEqual(o) && isLimitsEqual(o) && isScheduleEqual(o);
}

bool App::isNameEqual(const App &o) const
{
    return appName == o.appName;
}

bool App::isProcWild() const
{
    return applyParent || applyChild || killChild || killProcess;
}

bool App::hasZone() const
{
    return zones.accept_mask != 0 || zones.reject_mask != 0;
}
