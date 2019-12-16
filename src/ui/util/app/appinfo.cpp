#include "appinfo.h"

#include "apputil.h"

bool AppInfo::isFileModified(const QString &appPath) const
{
    return fileModTime != AppUtil::getModTime(appPath);
}
