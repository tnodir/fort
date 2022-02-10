#include "appinfo.h"

#include "appinfoutil.h"

QString AppInfo::getPath(const QString &appPath) const
{
    return !altPath.isEmpty() ? altPath : appPath;
}

bool AppInfo::isFileModified(const QString &appPath) const
{
    return fileModTime != AppInfoUtil::fileModTime(getPath(appPath));
}
