#ifndef NETUTIL_H
#define NETUTIL_H

#include <QObject>
#include <QString>

#include <common/common_types.h>

class NetUtil
{
public:
    // Convert IPv4 address from text to number
    static quint32 textToIp4(const QStringView &text, bool *ok = nullptr);
    static quint32 textToIp4(const char *text, bool *ok = nullptr);

    // Convert IPv4 address from number to text
    static QString ip4ToText(quint32 ip);

    // Convert IPv6 address from text to number
    static ip6_addr_t textToIp6(const QStringView &text, bool *ok = nullptr);
    static ip6_addr_t textToIp6(const char *text, bool *ok = nullptr);

    // Convert IPv6 address from number to text
    static QString ip6ToText(const ip6_addr_t &ip);

    static QString ipToText(const ip_addr_t &ip, bool isIPv6 = false);

    // Get IPv4 address mask
    static int ip4Mask(quint32 ip);

    static quint32 applyIp4Mask(quint32 ip, int nbits);
    static ip6_addr_t applyIp6Mask(const ip6_addr_t &ip, int nbits);

    static QByteArray ip6ToRawArray(const ip_addr_t &ip);
    static const ip6_addr_t &rawArrayToIp6(const QByteArray &buf);

    static QString formatDataSize(qint64 bytes, int precision = 2);
    static QString formatDataSize1(qint64 bytes);
    static QString formatSpeed(quint32 bitsPerSecond);

    static QString getHostName(const QString &address);

    static QStringList localIpNetworks();
    static QString localIpNetworksText();

    static QString protocolName(quint8 ipProto);
};

#endif // NETUTIL_H
