#include "appinfo.h"

#include "../fileutil.h"
#include "appinfoutil.h"

bool AppInfo::isFileModified(const QString &appPath) const
{
    return fileModTime != FileUtil::fileModTime(appPath);
}
