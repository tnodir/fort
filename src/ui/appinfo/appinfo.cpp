#include "appinfo.h"

#include "appinfoutil.h"

QString AppInfo::filePath(const QString &appPath) const
{
    return !altPath.isEmpty() ? altPath : appPath;
}

bool AppInfo::isFileModified(const QString &appPath) const
{
    const auto appFileModTime = AppInfoUtil::fileModTime(filePath(appPath));

    return appFileModTime.isValid() && appFileModTime != fileModTime;
}
