#include "appinfo.h"

#include "appinfoutil.h"

QString AppInfo::filePath(const QString &appPath) const
{
    return !altPath.isEmpty() ? altPath : appPath;
}

bool AppInfo::isFileModified(const QString &appPath) const
{
    return fileModTime != AppInfoUtil::fileModTime(filePath(appPath));
}
