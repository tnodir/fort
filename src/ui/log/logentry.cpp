#include "logentry.h"

#include <util/fileutil.h>
#include <util/osutil.h>

QString LogEntry::getAppPath(const QString &kernelPath, quint32 pid)
{
    return kernelPath.isEmpty() ? OsUtil::pidToPath(pid) : FileUtil::kernelPathToPath(kernelPath);
}
