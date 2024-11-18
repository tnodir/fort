#ifndef NETUTIL_H
#define NETUTIL_H

#include <QObject>
#include <QString>

#include <common/common_types.h>

enum IpProtocolType : qint8 {
    IpProto_ICMP = 1,
    IpProto_IGMP = 2,
    IpProto_TCP = 6,
    IpProto_UDP = 17,
    IpProto_ICMPV6 = 58,
};

class NetUtil
{
public:
    static bool windowsSockInit();
    static void windowsSockCleanup();

    // Get IPv4 address mask
    static int ip4Mask(quint32 ip);

    static quint32 applyIp4Mask(quint32 ip, int nbits);
    static ip6_addr_t applyIp6Mask(ip6_addr_t ip, int nbits);

    static QByteArrayView ip6ToArrayView(const ip6_addr_t &ip);
    static const ip6_addr_t &arrayViewToIp6(const QByteArrayView &buf);

    static QString getHostName(const QString &address);

    static QStringList localIpNetworks();
    static QString localIpNetworksText();

    static QString protocolName(quint8 ipProto);
    static quint8 protocolNumber(const QStringView name);

    static quint16 serviceToPort(const QStringView name, const char *proto, bool &ok);
};

#endif // NETUTIL_H
