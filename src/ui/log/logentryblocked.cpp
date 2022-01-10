#include "logentryblocked.h"

LogEntryBlocked::LogEntryBlocked(quint32 pid, const QString &kernelPath) :
    m_blocked(true),
    m_pid(pid),
    m_kernelPath(kernelPath)
{
}

FortLogType LogEntryBlocked::type() const
{
    return blocked() ? FORT_LOG_TYPE_BLOCKED : FORT_LOG_TYPE_ALLOWED;
}

void LogEntryBlocked::setBlocked(bool blocked)
{
    m_blocked = blocked;
}

void LogEntryBlocked::setPid(quint32 pid)
{
    m_pid = pid;
}

void LogEntryBlocked::setKernelPath(const QString &kernelPath)
{
    m_kernelPath = kernelPath;
}

QString LogEntryBlocked::path() const
{
    return getAppPath(m_kernelPath, m_pid);
}
