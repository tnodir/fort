#include "appinfo.h"

#include "appinfoutil.h"

bool AppInfo::isFileModified(const QString &appPath) const
{
    return fileModTime != AppInfoUtil::fileModTime(appPath);
}
