#include "netutil.h"

#include <QLocale>

#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>

#include <util/bitutil.h>

struct sock_addr
{
    union {
        struct sockaddr addr;
        struct sockaddr_in in;
    } u;
    int addrlen;
};

#define MAX_IPV4_LEN  16
#define MAX_IPV6_LEN  40
#define SOCK_ADDR_LEN offsetof(struct sock_addr, addrlen)

#define sock_addr_get_inp(sap) ((void *) &(sap)->u.in.sin_addr)

quint32 NetUtil::textToIp4(const QStringView &text, bool *ok)
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

quint32 NetUtil::textToIp4(const char *text, bool *ok)
{
    return textToIp4(QString::fromLatin1(text), ok);
}

QString NetUtil::ip4ToText(quint32 ip)
{
    quint32 ip4 = htonl((unsigned long) ip);
    wchar_t buf[MAX_IPV4_LEN];

    if (!InetNtopW(AF_INET, (PVOID) &ip4, buf, MAX_IPV4_LEN))
        return QString();

    return QString::fromWCharArray(buf);
}

ip6_addr_t NetUtil::textToIp6(const QStringView &text, bool *ok)
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

ip6_addr_t NetUtil::textToIp6(const char *text, bool *ok)
{
    return textToIp6(QString::fromLatin1(text), ok);
}

QString NetUtil::ip6ToText(const ip6_addr_t &ip)
{
    wchar_t buf[MAX_IPV6_LEN];

    if (!InetNtopW(AF_INET6, (PVOID) &ip, buf, MAX_IPV6_LEN))
        return QString();

    return QString::fromWCharArray(buf);
}

QString NetUtil::ipToText(const ip_addr_t &ip, bool isIPv6)
{
    return isIPv6 ? ip6ToText(ip.v6) : ip4ToText(ip.v4);
}

int NetUtil::ip4Mask(quint32 ip)
{
    return 32 - BitUtil::firstZeroBit(ip);
}

quint32 NetUtil::applyIp4Mask(quint32 ip, int nbits)
{
    return nbits == 0 ? quint32(-1) : (ip | (nbits == 32 ? 0 : ((1 << (32 - nbits)) - 1)));
}

ip6_addr_t NetUtil::applyIp6Mask(const ip6_addr_t &ip, int nbits)
{
    ip6_addr_t ip6 = ip;
    quint64 *masked = &ip6.hi64;

    if (nbits <= 64) {
        *masked = quint64(-1LL);
        masked = &ip6.lo64;
    } else {
        nbits -= 64;
    }

    *masked |= nbits == 0 ? quint64(-1LL) : (nbits == 64 ? 0 : htonll((1ULL << (64 - nbits)) - 1));

    return ip6;
}

QByteArray NetUtil::ip6ToRawArray(const ip_addr_t &ip)
{
    return QByteArray::fromRawData(ip.v6.data, sizeof(ip6_addr_t));
}

const ip6_addr_t &NetUtil::rawArrayToIp6(const QByteArray &buf)
{
    Q_ASSERT(buf.size() == sizeof(ip6_addr_t));
    return *reinterpret_cast<const ip6_addr_t *>(buf.data());
}

QString NetUtil::formatDataSize(qint64 bytes, int precision)
{
    return QLocale::c().formattedDataSize(bytes, precision, QLocale::DataSizeTraditionalFormat);
}

QString NetUtil::formatDataSize1(qint64 bytes)
{
    QString text = formatDataSize(bytes, 1);
    text.remove(QLatin1String(".0"));
    return text;
}

QString NetUtil::formatSpeed(quint32 bitsPerSecond)
{
    QString text = formatDataSize1(bitsPerSecond);

    if (bitsPerSecond < 1024) {
        text.replace("bytes", "b");
    } else {
        text.replace('B', 'b');
    }

    return text + QLatin1String("ps");
}

QString NetUtil::getHostName(const QString &address)
{
    WCHAR hostName[NI_MAXHOST];

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(textToIp4(address));

    if (GetNameInfoW((struct sockaddr *) &sa, sizeof(struct sockaddr), hostName, NI_MAXHOST,
                nullptr, 0, 0))
        return QString();

    return QString::fromWCharArray(hostName);
}

QStringList NetUtil::localIpNetworks()
{
    static QStringList list = {
        "0.0.0.0/32", // non-routable meta-address
        "10.0.0.0/8", //
        "100.64.0.0/10", // for carrier-grade NAT deployment
        "127.0.0.0/8", // Loopback
        "169.254.0.0/16", // if cannot obtain a network address via DHCP
        "172.16.0.0/12", //
        "192.168.0.0/16", //
        "239.255.255.250/32", // IP Multicast for DLNA/UPNP
        "255.255.255.255/32", // IP Broadcast
        "::/0", // non-routable meta-address
        "::/128", //
        "::1/128", // Localhost
        "::ffff:0:0/96", //
        "::ffff:0:0:0/96", //
        "64:ff9b::/96", //
        "100::/64", //
        "2001::/32", // Global Unique Addresses (GUA) - Routable IPv6 addresses
        "2001:20::/28", //
        "2001:db8::/32", // Documentation prefix used for examples
        "2002::/16", //
        "fc00::/7", // Unique Local Addresses (ULA) - also known as “Private” IPv6 addresses
        "fe80::/10", // Link Local addresses, only valid inside a single broadcast domain
        "ff00::/8", // Multicast addresses
    };

    return list;
}

QString NetUtil::localIpNetworksText()
{
    return NetUtil::localIpNetworks().join('\n') + '\n';
}

QString NetUtil::protocolName(quint8 ipProto)
{
    switch (ipProto) {
    case IPPROTO_ICMP:
        return "ICMP";
    case IPPROTO_IGMP:
        return "IGMP";
    case IPPROTO_TCP:
        return "TCP";
    case IPPROTO_UDP:
        return "UDP";
    case IPPROTO_ICMPV6:
        return "ICMPv6";
    default:
        return QString("0x%1").arg(ipProto, 0, 16);
    }
}
