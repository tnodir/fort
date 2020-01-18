#include "logentryblocked.h"

LogEntryBlocked::LogEntryBlocked(quint32 ip, quint16 port,
                                 quint8 proto, quint32 pid,
                                 const QString &kernelPath) :
    m_blocked(true),
    m_proto(proto),
    m_port(port),
    m_ip(ip),
    m_pid(pid),
    m_kernelPath(kernelPath)
{
}

void LogEntryBlocked::setBlocked(bool blocked)
{
    m_blocked = blocked;
}

void LogEntryBlocked::setProto(quint8 proto)
{
    m_proto = proto;
}

void LogEntryBlocked::setPort(quint16 port)
{
    m_port = port;
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

QString LogEntryBlocked::path() const
{
    return getAppPath(m_kernelPath, m_pid);
}
