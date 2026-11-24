#include "logentryprocstart.h"

LogEntryProcStart::LogEntryProcStart(quint32 pid, const QString &kernelPath) :
    m_pid(pid), m_kernelPath(kernelPath)
{
}

QString LogEntryProcStart::path() const
{
    return getAppPath(m_kernelPath, m_pid);
}
