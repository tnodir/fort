#include "app.h"

bool App::isEqual(const App &o) const
{
    return isFlagsEqual(o) && groupIndex == o.groupIndex && appOriginPath == o.appOriginPath
            && appPath == o.appPath && endTime == o.endTime;
}

bool App::isFlagsEqual(const App &o) const
{
    return isWildcard == o.isWildcard && useGroupPerm == o.useGroupPerm
            && applyChild == o.applyChild && killChild == o.killChild && lanOnly == o.lanOnly
            && logBlocked == o.logBlocked && logConn == o.logConn && blocked == o.blocked
            && killProcess == o.killProcess;
}
