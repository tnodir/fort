#include "app.h"

bool App::isEqual(const App &o) const
{
    return isWildcard == o.isWildcard && useGroupPerm == o.useGroupPerm
            && applyChild == o.applyChild && lanOnly == o.lanOnly && logBlocked == o.logBlocked
            && logConn == o.logConn && blocked == o.blocked && killProcess == o.killProcess
            && groupIndex == o.groupIndex && appOriginPath == o.appOriginPath
            && appPath == o.appPath && endTime == o.endTime;
}
