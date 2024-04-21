#include "logentryblocked.h"

LogEntryBlocked::LogEntryBlocked(quint32 pid, const QString &kernelPath) :
    m_pid(pid),
    m_kernelPath(kernelPath)
{
}

FortLogType LogEntryBlocked::type() const
{
    return blocked() ? FORT_LOG_TYPE_BLOCKED : FORT_LOG_TYPE_ALLOWED;
}

QString LogEntryBlocked::path() const
{
    return getAppPath(m_kernelPath, m_pid);
}
