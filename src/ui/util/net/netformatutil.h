#ifndef NETFORMATUTIL_H
#define NETFORMATUTIL_H

#include <QObject>
#include <QString>

#include <common/common_types.h>

class NetFormatUtil
{
public:
    // Convert IPv4 address from text to number
    static quint32 textToIp4(const QStringView text, bool *ok = nullptr);
    static quint32 textToIp4(const char *text, bool *ok = nullptr);

    // Convert IPv4 address from number to text
    static QString ip4ToText(quint32 ip);

    // Convert IPv6 address from text to number
    static ip6_addr_t textToIp6(const QStringView text, bool *ok = nullptr);
    static ip6_addr_t textToIp6(const char *text, bool *ok = nullptr);

    // Convert IPv6 address from number to text
    static QString ip6ToText(const ip6_addr_t ip);

    static QString ipToText(const ip_addr_t ip, bool isIPv6 = false);
};

#endif // NETFORMATUTIL_H
