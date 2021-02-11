#include "logentryblockedip.h"

LogEntryBlockedIp::LogEntryBlockedIp(quint8 blockReason, quint8 proto, quint16 localPort,
        quint16 remotePort, quint32 localIp, quint32 remoteIp, quint32 pid,
        const QString &kernelPath) :
    LogEntryBlocked(pid, kernelPath),
    m_blockReason(blockReason),
    m_proto(proto),
    m_localPort(localPort),
    m_remotePort(remotePort),
    m_localIp(localIp),
    m_remoteIp(remoteIp)
{
}

void LogEntryBlockedIp::setBlockReason(quint8 blockReason)
{
    m_blockReason = blockReason;
}

void LogEntryBlockedIp::setProto(quint8 proto)
{
    m_proto = proto;
}

void LogEntryBlockedIp::setLocalPort(quint16 port)
{
    m_localPort = port;
}

void LogEntryBlockedIp::setRemotePort(quint16 port)
{
    m_remotePort = port;
}

void LogEntryBlockedIp::setLocalIp(quint32 ip)
{
    m_localIp = ip;
}

void LogEntryBlockedIp::setRemoteIp(quint32 ip)
{
    m_remoteIp = ip;
}
