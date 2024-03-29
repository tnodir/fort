#include "logentryblockedip.h"

#include <util/net/netutil.h>

void LogEntryBlockedIp::setIsIPv6(bool isIPv6)
{
    m_isIPv6 = isIPv6;
}

void LogEntryBlockedIp::setInbound(bool inbound)
{
    m_inbound = inbound;
}

void LogEntryBlockedIp::setInherited(bool inherited)
{
    m_inherited = inherited;
}

void LogEntryBlockedIp::setBlockReason(quint8 blockReason)
{
    m_blockReason = blockReason;
}

void LogEntryBlockedIp::setIpProto(quint8 proto)
{
    m_ipProto = proto;
}

void LogEntryBlockedIp::setLocalPort(quint16 port)
{
    m_localPort = port;
}

void LogEntryBlockedIp::setRemotePort(quint16 port)
{
    m_remotePort = port;
}

void LogEntryBlockedIp::setConnTime(qint64 connTime)
{
    m_connTime = connTime;
}

void LogEntryBlockedIp::setLocalIp(ip_addr_t &ip)
{
    m_localIp = ip;
}

void LogEntryBlockedIp::setLocalIp4(quint32 ip)
{
    m_localIp.v4 = ip;
}

QByteArray LogEntryBlockedIp::localIp6() const
{
    return NetUtil::ip6ToRawArray(m_localIp);
}

void LogEntryBlockedIp::setLocalIp6(const QByteArray &ip)
{
    m_localIp.v6 = NetUtil::rawArrayToIp6(ip);
}

void LogEntryBlockedIp::setRemoteIp(ip_addr_t &ip)
{
    m_remoteIp = ip;
}

void LogEntryBlockedIp::setRemoteIp4(quint32 ip)
{
    m_remoteIp.v4 = ip;
}

QByteArray LogEntryBlockedIp::remoteIp6() const
{
    return NetUtil::ip6ToRawArray(m_remoteIp);
}

void LogEntryBlockedIp::setRemoteIp6(const QByteArray &ip)
{
    m_remoteIp.v6 = NetUtil::rawArrayToIp6(ip);
}

bool LogEntryBlockedIp::isAskPending() const
{
    return blockReason() == FORT_BLOCK_REASON_ASK_PENDING;
}
