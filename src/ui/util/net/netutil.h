#ifndef NETUTIL_H
#define NETUTIL_H

#include <QObject>
#include <QString>

#include <common/common_types.h>

class NetUtil
{
public:
    static bool windowsSockInit();
    static void windowsSockCleanup();

    // Convert IPv4 address from text to number
    static quint32 textToIp4(const QStringView &text, bool *ok = nullptr);
    static quint32 textToIp4(const char *text, bool *ok = nullptr);

    // Convert IPv4 address from number to text
    static QString ip4ToText(quint32 ip);

    // Convert IPv6 address from text to number
    static ip6_addr_t textToIp6(const QStringView &text, bool *ok = nullptr);
    static ip6_addr_t textToIp6(const char *text, bool *ok = nullptr);

    // Convert IPv6 address from number to text
    static QString ip6ToText(const ip6_addr_t ip);

    static QString ipToText(const ip_addr_t ip, bool isIPv6 = false);

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
    static quint8 protocolNumber(const QStringView &name);

    static quint16 serviceToPort(const QStringView &name, const char *proto, bool &ok);
};

#endif // NETUTIL_H
