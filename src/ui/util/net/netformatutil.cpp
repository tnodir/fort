#include "netformatutil.h"

#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>

struct sock_addr
{
    union {
        struct sockaddr addr;
        struct sockaddr_in in;
    } u;
    int addrlen;
};

#define MAX_IPV4_LEN 16
#define MAX_IPV6_LEN 40

#define SOCK_ADDR_LEN offsetof(struct sock_addr, addrlen)

#define sock_addr_get_inp(sap) ((void *) &(sap)->u.in.sin_addr)

quint32 NetFormatUtil::textToIp4(const QStringView text, bool *ok)
{
    quint32 ip4;

    const bool res = InetPtonW(AF_INET, (PCWSTR) text.utf16(), &ip4) == 1;

    if (ok) {
        *ok = res;
    } else if (!res) {
        ip4 = 0;
    }

    return ntohl(*((unsigned long *) &ip4));
}

quint32 NetFormatUtil::textToIp4(const char *text, bool *ok)
{
    return textToIp4(QString::fromLatin1(text), ok);
}

QString NetFormatUtil::ip4ToText(quint32 ip)
{
    quint32 ip4 = htonl((unsigned long) ip);
    wchar_t buf[MAX_IPV4_LEN];

    if (!InetNtopW(AF_INET, (PVOID) &ip4, buf, MAX_IPV4_LEN))
        return QString();

    return QString::fromWCharArray(buf);
}

ip6_addr_t NetFormatUtil::textToIp6(const QStringView text, bool *ok)
{
    ip6_addr_t ip6;

    const bool res = InetPtonW(AF_INET6, (PCWSTR) text.utf16(), &ip6) == 1;

    if (ok) {
        *ok = res;
    } else if (!res) {
        memset(&ip6, 0, sizeof(ip6));
    }

    return ip6;
}

ip6_addr_t NetFormatUtil::textToIp6(const char *text, bool *ok)
{
    return textToIp6(QString::fromLatin1(text), ok);
}

QString NetFormatUtil::ip6ToText(const ip6_addr_t ip)
{
    wchar_t buf[MAX_IPV6_LEN];

    if (!InetNtopW(AF_INET6, (PVOID) &ip, buf, MAX_IPV6_LEN))
        return QString();

    return QString::fromWCharArray(buf);
}

QString NetFormatUtil::ipToText(const ip_addr_t ip, bool isIPv6)
{
    return isIPv6 ? ip6ToText(ip.v6) : ip4ToText(ip.v4);
}
