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
    return acceptZones == o.acceptZones && rejectZones == o.rejectZones;
}

bool App::isPathsEqual(const App &o) const
{
    return appOriginPath == o.appOriginPath && appPath == o.appPath;
}

bool App::isOptionsEqual(const App &o) const
{
    return isFlagsEqual(o) && isZonesEqual(o) && groupIndex == o.groupIndex && ruleId == o.ruleId
            && isPathsEqual(o) && notes == o.notes && scheduleAction == o.scheduleAction
            && scheduleTime == o.scheduleTime;
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
    return acceptZones != 0 || rejectZones != 0;
}
