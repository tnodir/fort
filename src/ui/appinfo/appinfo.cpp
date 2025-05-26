#include "appinfo.h"

#include "appinfoutil.h"

QString AppInfo::filePath(const QString &appPath) const
{
    return !altPath.isEmpty() ? altPath : appPath;
}

bool AppInfo::checkFileModified(const QString &appPath)
{
    const auto appFileModTime = AppInfoUtil::fileModTime(filePath(appPath), fileExists);

    return appFileModTime.isValid() && appFileModTime != fileModTime;
}
