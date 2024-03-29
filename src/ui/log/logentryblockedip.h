#ifndef LOGENTRYBLOCKEDIP_H
#define LOGENTRYBLOCKEDIP_H

#include <common/common_types.h>

#include "logentryblocked.h"

class LogEntryBlockedIp : public LogEntryBlocked
{
public:
    FortLogType type() const override { return FORT_LOG_TYPE_BLOCKED_IP; }

    bool isIPv6() const { return m_isIPv6; }
    void setIsIPv6(bool isIPv6);

    bool inbound() const { return m_inbound; }
    void setInbound(bool inbound);

    bool inherited() const { return m_inherited; }
    void setInherited(bool inherited);

    quint8 blockReason() const { return m_blockReason; }
    void setBlockReason(quint8 blockReason);

    quint8 ipProto() const { return m_ipProto; }
    void setIpProto(quint8 proto);

    quint16 localPort() const { return m_localPort; }
    void setLocalPort(quint16 port);

    quint16 remotePort() const { return m_remotePort; }
    void setRemotePort(quint16 port);

    qint64 connTime() const { return m_connTime; }
    void setConnTime(qint64 connTime);

    const ip_addr_t &localIp() const { return m_localIp; }
    ip_addr_t &localIp() { return m_localIp; }
    void setLocalIp(ip_addr_t &ip);

    quint32 localIp4() const { return m_localIp.v4; }
    void setLocalIp4(quint32 ip);

    QByteArray localIp6() const;
    void setLocalIp6(const QByteArray &ip);

    const ip_addr_t &remoteIp() const { return m_remoteIp; }
    ip_addr_t &remoteIp() { return m_remoteIp; }
    void setRemoteIp(ip_addr_t &ip);

    quint32 remoteIp4() const { return m_remoteIp.v4; }
    void setRemoteIp4(quint32 ip);

    QByteArray remoteIp6() const;
    void setRemoteIp6(const QByteArray &ip);

    bool isAskPending() const;

private:
    bool m_isIPv6 : 1 = false;
    bool m_inbound : 1 = false;
    bool m_inherited : 1 = false;
    quint8 m_blockReason = 0;
    quint8 m_ipProto = 0;
    quint16 m_localPort = 0;
    quint16 m_remotePort = 0;
    qint64 m_connTime = 0;
    ip_addr_t m_localIp;
    ip_addr_t m_remoteIp;
};

#endif // LOGENTRYBLOCKEDIP_H
