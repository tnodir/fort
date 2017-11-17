#include "logentryblocked.h"

LogEntryBlocked::LogEntryBlocked(quint32 ip, quint32 pid,
                                 const QString &kernelPath) :
    LogEntry(),
    m_ip(ip),
    m_pid(pid),
    m_kernelPath(kernelPath)
{
}

void LogEntryBlocked::setIp(quint32 ip)
{
    m_ip = ip;
}

void LogEntryBlocked::setPid(quint32 pid)
{
    m_pid = pid;
}

void LogEntryBlocked::setKernelPath(const QString &kernelPath)
{
    m_kernelPath = kernelPath;
}
