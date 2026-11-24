#include "logentryprocnew.h"

#include <util/fileutil.h>

LogEntryProcNew::LogEntryProcNew(quint32 pid, const QString &kernelPath) :
    LogEntryProcStart(pid, kernelPath)
{
}

QString LogEntryProcNew::inheritPath() const
{
    return FileUtil::kernelPathToPath(m_inheritKernelPath);
}
