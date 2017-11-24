#include "logentryprocnew.h"

LogEntryProcNew::LogEntryProcNew(quint32 pid,
                                 const QString &kernelPath) :
    LogEntry(),
    m_pid(pid),
    m_kernelPath(kernelPath)
{
}

void LogEntryProcNew::setPid(quint32 pid)
{
    m_pid = pid;
}

void LogEntryProcNew::setKernelPath(const QString &kernelPath)
{
    m_kernelPath = kernelPath;
}
