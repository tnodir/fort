#include "logentryprocpath.h"

#include <util/fileutil.h>
#include <util/osutil.h>

LogEntryProcPath::LogEntryProcPath(quint32 pid, const QString &kernelPath) :
    LogEntryProc(pid), m_kernelPath(kernelPath)
{
}

QString LogEntryProcPath::path() const
{
    return m_kernelPath.isEmpty() ? OsUtil::pidToPath(pid())
                                  : FileUtil::kernelPathToPath(m_kernelPath);
}
