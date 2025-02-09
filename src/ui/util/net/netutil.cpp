#include "netutil.h"

#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>

#include <util/bitutil.h>

#include "netformatutil.h"

bool NetUtil::windowsSockInit()
{
    WSAData wsadata;
    return WSAStartup(MAKEWORD(2, 2), &wsadata) == 0;
}

void NetUtil::windowsSockCleanup()
{
    WSACleanup();
}

int NetUtil::ip4Mask(quint32 ip)
{
    return 32 - BitUtil::firstZeroBit(ip);
}

quint32 NetUtil::applyIp4Mask(quint32 ip, int nbits)
{
    return nbits == 0 ? quint32(-1) : (ip | (nbits == 32 ? 0 : ((1 << (32 - nbits)) - 1)));
}

ip6_addr_t NetUtil::applyIp6Mask(ip6_addr_t ip, int nbits)
{
    quint64 *masked = &ip.hi64;

    if (nbits <= 64) {
        *masked = quint64(-1LL);
        masked = &ip.lo64;
    } else {
        nbits -= 64;
    }

    *masked |= nbits == 0 ? quint64(-1LL) : (nbits == 64 ? 0 : htonll((1ULL << (64 - nbits)) - 1));

    return ip;
}

QByteArrayView NetUtil::ip6ToArrayView(const ip6_addr_t &ip)
{
    return QByteArrayView(ip.data, sizeof(ip6_addr_t));
}

const ip6_addr_t &NetUtil::arrayViewToIp6(const QByteArrayView &buf)
{
    Q_ASSERT(buf.size() == sizeof(ip6_addr_t));
    return *reinterpret_cast<const ip6_addr_t *>(buf.data());
}

QString NetUtil::getHostName(const QString &address)
{
    WCHAR hostName[NI_MAXHOST];

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(NetFormatUtil::textToIp4(address));

    if (GetNameInfoW((struct sockaddr *) &sa, sizeof(struct sockaddr), hostName, NI_MAXHOST,
                nullptr, 0, 0))
        return QString();

    return QString::fromWCharArray(hostName);
}

QStringList NetUtil::localIpNetworks()
{
    static QStringList list = {
        "0.0.0.0/32", // non-routable meta-address
        "10.0.0.0/8", // Private use
        "100.64.0.0/10", // Carrier-grade NAT deployment
        "127.0.0.0/8", // Loopback
        "169.254.0.0/16", // Link Local (if cannot obtain a network address via DHCP)
        "172.16.0.0/12", // Private use
        "192.168.0.0/16", // Private use
        "198.18.0.0/15", // Benchmarking
        "224.0.0.0/24", // Multicast addresses
        "239.255.255.250/32", // IP Multicast for DLNA/UPNP
        "255.255.255.255/32", // IP Broadcast
        "::/0", // non-routable meta-address
        "::/128", // Unspecified Address
        "::1/128", // Loopback
        "::ffff:0:0/96", // IPv4-mapped Address
        "::ffff:0:0:0/96", //
        "64:ff9b::/96", // IPv4-IPv6 Translat.
        "100::/64", // Discard-Only Address Block
        "2001::/32", // Global Unique Addresses (GUA) - Routable IPv6 addresses
        "2001:20::/28", // ORCHIDv2
        "2001:db8::/32", // Documentation prefix used for examples
        "2002::/16", // 6to4
        "fc00::/7", // Unique Local Addresses (ULA) - also known as "Private" IPv6 addresses
        "fe80::/10", // Link Local addresses, only valid inside a single broadcast domain
        "ff00::/8", // Multicast addresses
    };

    return list;
}

QString NetUtil::localIpNetworksText(int count)
{
    return NetUtil::localIpNetworks().mid(0, count).join('\n') + '\n';
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
        break;
    }

    const protoent *pe = getprotobynumber(ipProto);
    if (pe) {
        return QString::fromLatin1(pe->p_name);
    }

    return QString::number(ipProto);
}

quint8 NetUtil::protocolNumber(const QStringView name)
{
    const auto nameUpper = name.toString().toUpper();

    if (nameUpper == "TCP")
        return IPPROTO_TCP;
    if (nameUpper == "UDP")
        return IPPROTO_UDP;
    if (nameUpper == "ICMP")
        return IPPROTO_ICMP;
    if (nameUpper == "ICMPV6")
        return IPPROTO_ICMPV6;
    if (nameUpper == "IGMP")
        return IPPROTO_IGMP;

    const QByteArray nameData = nameUpper.toLatin1();

    const protoent *pe = getprotobyname(nameData.constData());
    if (pe) {
        return quint8(pe->p_proto);
    }

    return 0;
}

quint16 NetUtil::serviceToPort(const QStringView name, const char *proto, bool &ok)
{
    const QByteArray nameData = name.toLatin1();

    const servent *se = getservbyname(nameData.constData(), proto);
    if (!se) {
        ok = false;
        return 0;
    }

    ok = true;
    return ntohs(se->s_port);
}
