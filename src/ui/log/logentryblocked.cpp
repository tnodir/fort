#include "logentryblocked.h"

LogEntryBlocked::LogEntryBlocked(quint32 ip, quint32 pid,
                                 const QString &kernelPath,
                                 QObject *parent) :
    QObject(parent),
    m_ip(ip),
    m_pid(pid),
    m_kernelPath(kernelPath)
{
}

void LogEntryBlocked::setIp(quint32 ip)
{
    if (m_ip != ip) {
        m_ip = ip;
        emit ipChanged();
    }
}

void LogEntryBlocked::setPid(quint32 pid)
{
    if (m_pid != pid) {
        m_pid = pid;
        emit pidChanged();
    }
}

void LogEntryBlocked::setKernelPath(const QString &kernelPath)
{
    if (m_kernelPath != kernelPath) {
        m_kernelPath = kernelPath;
        emit kernelPathChanged();
    }
}
