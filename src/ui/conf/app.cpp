#include "app.h"

bool App::isFlagsEqual(const App &o) const
{
    return isBaseFlagsEqual(o) && isExtraFlagsEqual(o);
}

bool App::isBaseFlagsEqual(const App &o) const
{
    return isWildcard == o.isWildcard && useGroupPerm == o.useGroupPerm
            && applyChild == o.applyChild && killChild == o.killChild && lanOnly == o.lanOnly;
}

bool App::isZonesEqual(const App &o) const
{
    return acceptZones == o.acceptZones && rejectZones == o.rejectZones;
}

bool App::isExtraFlagsEqual(const App &o) const
{
    return logBlocked == o.logBlocked && logConn == o.logConn && blocked == o.blocked
            && killProcess == o.killProcess;
}

bool App::isOptionsEqual(const App &o) const
{
    return isFlagsEqual(o) && isZonesEqual(o) && groupIndex == o.groupIndex
            && appOriginPath == o.appOriginPath && appPath == o.appPath && endTime == o.endTime;
}

bool App::isNameEqual(const App &o) const
{
    return appName == o.appName;
}

bool App::isProcWild() const
{
    return applyChild || killChild || killProcess;
}
