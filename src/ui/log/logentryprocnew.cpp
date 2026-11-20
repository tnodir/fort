#include "logentryprocnew.h"

#include <util/fileutil.h>

LogEntryProcNew::LogEntryProcNew(quint32 pid, const QString &kernelPath) :
    m_pid(pid), m_kernelPath(kernelPath)
{
}

QString LogEntryProcNew::path() const
{
    return getAppPath(m_kernelPath, m_pid);
}

QString LogEntryProcNew::inheritPath() const
{
    return FileUtil::kernelPathToPath(m_inheritKernelPath);
}
