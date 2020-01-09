#include "appinfo.h"

#include "../fileutil.h"
#include "apputil.h"

bool AppInfo::isFileModified(const QString &appPath) const
{
    return fileModTime != FileUtil::fileModTime(appPath);
}
