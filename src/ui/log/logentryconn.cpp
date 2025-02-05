#include "logentryconn.h"

#include <util/net/netutil.h>

void LogEntryConn::setLocalIp4(quint32 ip)
{
    m_localIp.v4 = ip;
}

QByteArrayView LogEntryConn::localIp6View() const
{
    return NetUtil::ip6ToArrayView(m_localIp.v6);
}

void LogEntryConn::setLocalIp6ByView(const QByteArrayView &ip)
{
    m_localIp.v6 = NetUtil::arrayViewToIp6(ip);
}

void LogEntryConn::setRemoteIp4(quint32 ip)
{
    m_remoteIp.v4 = ip;
}

QByteArrayView LogEntryConn::remoteIp6View() const
{
    return NetUtil::ip6ToArrayView(m_remoteIp.v6);
}

void LogEntryConn::setRemoteIp6ByView(const QByteArrayView &ip)
{
    m_remoteIp.v6 = NetUtil::arrayViewToIp6(ip);
}

bool LogEntryConn::isAskPending() const
{
    return reason() == FORT_CONN_REASON_ASK_PENDING;
}
