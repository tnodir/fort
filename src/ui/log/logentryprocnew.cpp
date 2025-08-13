#include "logentryprocnew.h"

LogEntryProcNew::LogEntryProcNew(quint32 pid, const QString &kernelPath) :
    m_pid(pid), m_kernelPath(kernelPath)
{
}

QString LogEntryProcNew::path() const
{
    return getAppPath(m_kernelPath, m_pid);
}
