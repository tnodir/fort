#include "logentry.h"

LogEntry::LogEntry(quint32 ip, quint32 pid,
                   const QString &kernelPath,
                   QObject *parent) :
    QObject(parent),
    m_ip(ip),
    m_pid(pid),
    m_kernelPath(kernelPath)
{
}

void LogEntry::setIp(quint32 ip)
{
    if (m_ip != ip) {
        m_ip = ip;
        emit ipChanged();
    }
}

void LogEntry::setPid(quint32 pid)
{
    if (m_pid != pid) {
        m_pid = pid;
        emit pidChanged();
    }
}

void LogEntry::setKernelPath(const QString &kernelPath)
{
    if (m_kernelPath != kernelPath) {
        m_kernelPath = kernelPath;
        emit kernelPathChanged();
    }
}
