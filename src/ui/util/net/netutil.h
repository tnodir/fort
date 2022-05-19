#ifndef NETUTIL_H
#define NETUTIL_H

#include <QObject>
#include <QString>

#include <common/common_types.h>

class NetUtil
{
public:
    // Convert IPv4 address from text to number
    static quint32 textToIp4(const QString &text, bool *ok = nullptr);

    // Convert IPv4 address from number to text
    static QString ip4ToText(quint32 ip);

    // Convert IPv6 address from text to number
    static ip6_addr_t textToIp6(const QString &text, bool *ok = nullptr);

    // Convert IPv6 address from number to text
    static QString ip6ToText(ip6_addr_t ip);

    // Get IPv4 address mask
    static int ip4Mask(quint32 ip);

    static QString formatDataSize(qint64 bytes, int precision = 2);
    static QString formatDataSize1(qint64 bytes);
    static QString formatSpeed(quint32 bytes);

    static QString getHostName(const QString &address);

    static QStringList localIpNetworks();

    static QString protocolName(quint8 ipProto);

private:
    static int first0Bit(quint32 u);
    static int bitCount(quint32 u);
};

#endif // NETUTIL_H
