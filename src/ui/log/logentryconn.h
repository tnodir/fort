#ifndef LOGENTRYCONN_H
#define LOGENTRYCONN_H

#include <common/common_types.h>

#include "logentryapp.h"

class LogEntryConn : public LogEntryApp
{
public:
    FortLogType type() const override { return FORT_LOG_TYPE_CONN; }

    bool isIPv6() const { return m_isIPv6; }
    void setIsIPv6(bool isIPv6) { m_isIPv6 = isIPv6; }

    bool inbound() const { return m_inbound; }
    void setInbound(bool inbound) { m_inbound = inbound; }

    bool inherited() const { return m_inherited; }
    void setInherited(bool inherited) { m_inherited = inherited; }

    quint8 reason() const { return m_reason; }
    void setReason(quint8 reason) { m_reason = reason; }

    quint8 ipProto() const { return m_ipProto; }
    void setIpProto(quint8 proto) { m_ipProto = proto; }

    quint8 zoneId() const { return m_zoneId; }
    void setZoneId(quint8 zoneId) { m_zoneId = zoneId; }

    quint16 ruleId() const { return m_ruleId; }
    void setRuleId(quint16 ruleId) { m_ruleId = ruleId; }

    quint16 localPort() const { return m_localPort; }
    void setLocalPort(quint16 port) { m_localPort = port; }

    quint16 remotePort() const { return m_remotePort; }
    void setRemotePort(quint16 port) { m_remotePort = port; }

    quint32 appId() const { return m_appId; }
    void setAppId(quint32 v) { m_appId = v; }

    qint64 connTime() const { return m_connTime; }
    void setConnTime(qint64 connTime) { m_connTime = connTime; }

    const ip_addr_t &localIp() const { return m_localIp; }
    ip_addr_t &localIp() { return m_localIp; }
    void setLocalIp(const ip_addr_t ip) { m_localIp = ip; }

    quint32 localIp4() const { return m_localIp.v4; }
    void setLocalIp4(quint32 ip);

    QByteArrayView localIp6View() const;
    void setLocalIp6ByView(const QByteArrayView &ip);

    const ip_addr_t &remoteIp() const { return m_remoteIp; }
    ip_addr_t &remoteIp() { return m_remoteIp; }
    void setRemoteIp(const ip_addr_t ip) { m_remoteIp = ip; }

    quint32 remoteIp4() const { return m_remoteIp.v4; }
    void setRemoteIp4(quint32 ip);

    QByteArrayView remoteIp6View() const;
    void setRemoteIp6ByView(const QByteArrayView &ip);

    bool isAskPending() const;

private:
    bool m_isIPv6 : 1 = false;
    bool m_inbound : 1 = false;
    bool m_inherited : 1 = false;
    quint8 m_reason = 0;
    quint8 m_ipProto = 0;
    quint8 m_zoneId = 0;
    quint16 m_ruleId = 0;
    quint16 m_localPort = 0;
    quint16 m_remotePort = 0;
    quint32 m_appId = 0;
    qint64 m_connTime = 0;
    ip_addr_t m_localIp;
    ip_addr_t m_remoteIp;
};

#endif // LOGENTRYCONN_H
