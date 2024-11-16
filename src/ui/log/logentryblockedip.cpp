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

void LogEntryBlockedIp::setLocalIp(const ip_addr_t ip)
{
    m_localIp = ip;
}

void LogEntryBlockedIp::setLocalIp4(quint32 ip)
{
    m_localIp.v4 = ip;
}

QByteArrayView LogEntryBlockedIp::localIp6View() const
{
    return NetUtil::ip6ToArrayView(m_localIp.v6);
}

void LogEntryBlockedIp::setLocalIp6ByView(const QByteArrayView &ip)
{
    m_localIp.v6 = NetUtil::arrayViewToIp6(ip);
}

void LogEntryBlockedIp::setRemoteIp(const ip_addr_t ip)
{
    m_remoteIp = ip;
}

void LogEntryBlockedIp::setRemoteIp4(quint32 ip)
{
    m_remoteIp.v4 = ip;
}

QByteArrayView LogEntryBlockedIp::remoteIp6View() const
{
    return NetUtil::ip6ToArrayView(m_remoteIp.v6);
}

void LogEntryBlockedIp::setRemoteIp6ByView(const QByteArrayView &ip)
{
    m_remoteIp.v6 = NetUtil::arrayViewToIp6(ip);
}

bool LogEntryBlockedIp::isAskPending() const
{
    return blockReason() == FORT_BLOCK_REASON_ASK_PENDING;
}
