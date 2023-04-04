#include "app.h"

bool App::isEqual(const App &o) const
{
    return useGroupPerm == o.useGroupPerm && applyChild == o.applyChild && lanOnly == o.lanOnly
            && logBlocked == o.logBlocked && logConn == o.logConn && blocked == o.blocked
            && groupIndex == o.groupIndex && appPath == o.appPath && endTime == o.endTime;
}
