#include "appinfo.h"

#include "../util/fileutil.h"

bool AppInfo::isFileModified(const QString &appPath) const
{
    return fileModTime != FileUtil::fileModTime(appPath);
}
