#include "netutil.h"

#include <QHash>

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
    }

    return QString::number(ipProto);
}

quint8 NetUtil::protocolNumber(const QStringView name)
{
    static QHash<QString, quint8> protocolNameNumbersMap = {
        { "HOPOPT", 0 }, // IPv6 Hop-by-Hop Option
        { "ICMP", 1 }, // Internet Control Message Protocol
        { "IGMP", 2 }, // Internet Group Management Protocol
        { "GGP", 3 }, // Gateway-to-Gateway Protocol
        { "IP-IN-IP", 4 }, // IP in IP (encapsulation)
        { "ST", 5 }, // Internet Stream Protocol
        { "TCP", 6 }, // Transmission Control Protocol
        { "CBT", 7 }, // Core-based trees
        { "EGP", 8 }, // Exterior Gateway Protocol
        { "IGP", 9 }, // Interior gateway protocol (any private interior gateway, eg. Cisco's IGRP)
        { "BBN-RCC-MON", 10 }, // BBN RCC Monitoring
        { "NVP-II", 11 }, // Network Voice Protocol
        { "PUP", 12 }, // PARC Universal Packet|Xerox PUP
        { "ARGUS", 13 }, // ARGUS
        { "EMCON", 14 }, // EMCON
        { "XNET", 15 }, // Cross Net Debugger
        { "CHAOS", 16 }, // Chaosnet|Chaos
        { "UDP", 17 }, // User Datagram Protocol
        { "MUX", 18 }, // Multiplexing
        { "DCN-MEAS", 19 }, // DCN Measurement Subsystems
        { "HMP", 20 }, // Host Monitoring Protocol
        { "PRM", 21 }, // Packet Radio Measurement
        { "XNS-IDP", 22 }, // XEROX NS IDP
        { "TRUNK-1", 23 }, // Trunk-1
        { "TRUNK-2", 24 }, // Trunk-2
        { "LEAF-1", 25 }, // Leaf-1
        { "LEAF-2", 26 }, // Leaf-2
        { "RDP", 27 }, // Reliable Data Protocol
        { "IRTP", 28 }, // Internet Reliable Transaction Protocol
        { "ISO-TP4", 29 }, // ISO Transport Protocol Class 4
        { "NETBLT", 30 }, // Bulk Data Transfer Protocol
        { "MFE-NSP", 31 }, // MFE Network Services Protocol
        { "MERIT-INP", 32 }, // MERIT Internodal Protocol
        { "DCCP", 33 }, // Datagram Congestion Control Protocol
        { "3PC", 34 }, // Third Party Connect Protocol
        { "IDPR", 35 }, // Inter-Domain Policy Routing Protocol
        { "XTP", 36 }, // Xpress Transport Protocol
        { "DDP", 37 }, // Datagram Delivery Protocol
        { "IDPR-CMTP", 38 }, // IDPR Control Message Transport Protocol
        { "TP++", 39 }, // TP++ Transport Protocol
        { "IL", 40 }, // IL (network protocol)|IL Transport Protocol
        { "IPV6", 41 }, // IPv6 Encapsulation (6to4 and 6in4)
        { "SDRP", 42 }, // Source Demand Routing Protocol
        { "IPV6-ROUTE", 43 }, // Routing Header for IPv6
        { "IPV6-FRAG", 44 }, // Fragment Header for IPv6
        { "IDRP", 45 }, // Inter-Domain Routing Protocol
        { "RSVP", 46 }, // Resource Reservation Protocol
        { "GRE", 47 }, // Generic Routing Encapsulation
        { "DSR", 48 }, // Dynamic Source Routing Protocol
        { "BNA", 49 }, // Burroughs Network Architecture
        { "ESP", 50 }, // Encapsulating Security Payload
        { "AH", 51 }, // Authentication Header
        { "I-NLSP", 52 }, // Integrated Net Layer Security Protocol
        { "SWIPE", 53 }, // SwIPe (protocol)|SwIPe
        { "NARP", 54 }, // NBMA Address Resolution Protocol
        { "MOBILE", 55 }, // Mobile IP|IP Mobility (Min Encap)
        { "TLSP", 56 }, // Transport Layer Security Protocol (using Kryptonet key management)
        { "SKIP", 57 }, // Simple Key-Management for Internet Protocol
        { "IPV6-ICMP", 58 }, // ICMP for IPv6
        { "IPV6-NONXT", 59 }, // No Next Header for IPv6
        { "IPV6-OPTS", 60 }, // Destination Options for IPv6
        { "ANY-HOST", 61 }, // Any host internal protocol
        { "CFTP", 62 }, // CFTP
        { "ANY-LAN", 63 }, // Any local network
        { "SAT-EXPAK", 64 }, // SATNET and Backroom EXPAK
        { "KRYPTOLAN", 65 }, // Kryptolan
        { "RVD", 66 }, // MIT Remote Virtual Disk Protocol
        { "IPPC", 67 }, // Internet Pluribus Packet Core
        { "ANY-DFS", 68 }, // Any distributed file system
        { "SAT-MON", 69 }, // SATNET Monitoring
        { "VISA", 70 }, // VISA Protocol
        { "IPCU", 71 }, // Internet Packet Core Utility
        { "CPNX", 72 }, // Computer Protocol Network Executive
        { "CPHB", 73 }, //
        { "WSN", 74 }, // Wang Span Network
        { "PVP", 75 }, // Packet Video Protocol
        { "BR-SAT-MON", 76 }, // Backroom SATNET Monitoring
        { "SUN-ND", 77 }, // SUN ND PROTOCOL-Temporary
        { "WB-MON", 78 }, // WIDEBAND Monitoring
        { "WB-EXPAK", 79 }, // WIDEBAND EXPAK
        { "ISO-IP", 80 }, // International Organization for Standardization Internet Protocol
        { "VMTP", 81 }, // Versatile Message Transaction Protocol
        { "SECURE-VMTP", 82 }, // Secure Versatile Message Transaction Protocol
        { "VINES", 83 }, // VINES
        { "IPTM", 84 }, // Internet Protocol Traffic Manager
        { "NSFNET-IGP", 85 }, // NSFNET-IGP
        { "DGP", 86 }, // Dissimilar Gateway Protocol
        { "TCF", 87 }, // TCF
        { "EIGRP", 88 }, // EIGRP
        { "OSPF", 89 }, // Open Shortest Path First
        { "SPRITE-RPC", 90 }, // Sprite RPC Protocol
        { "LARP", 91 }, // Locus Address Resolution Protocol
        { "MTP", 92 }, // Multicast Transport Protocol
        { "AX.25", 93 }, // AX.25
        { "OS", 94 }, // KA9Q|KA9Q NOS compatible IP over IP tunneling
        { "MICP", 95 }, // Mobile Internetworking Control Protocol
        { "SCC-SP", 96 }, // Semaphore Communications Sec. Pro
        { "ETHERIP", 97 }, // Ethernet-within-IP Encapsulation
        { "ENCAP", 98 }, // Encapsulation Header
        { "ANY-PRIV-ENC", 99 }, // Any private encryption scheme
        { "GMTP", 100 }, // GMTP
        { "IFMP", 101 }, // Ipsilon Flow Management Protocol
        { "PNNI", 102 }, // Private Network-to-Network Interface|PNNI over IP
        { "PIM", 103 }, // Protocol Independent Multicast
        { "ARIS", 104 }, // IBM's ARIS (Aggregate Route IP Switching) Protocol
        { "SCPS", 105 }, // Space Communications Protocol Specifications
        { "QNX", 106 }, // QNX
        { "A/N", 107 }, // Active Networks
        { "IPCOMP", 108 }, // IP Payload Compression Protocol
        { "SNP", 109 }, // Sitara Networks Protocol
        { "COMPAQ-PEER", 110 }, // Compaq Peer Protocol
        { "IPX-IN-IP", 111 }, // IPX in IP
        { "VRRP", 112 }, // Virtual Router Redundancy Protocol
        { "PGM", 113 }, // Pragmatic General Multicast (PGM) Reliable Transport Protocol
        { "ANY-0HOP", 114 }, // Any 0-hop protocol
        { "L2TP", 115 }, // (L2TPv3) Layer Two Tunneling Protocol Version 3
        { "DDX", 116 }, // D-II Data Exchange (DDX)
        { "IATP", 117 }, // Interactive Agent Transfer Protocol
        { "STP", 118 }, // Schedule Transfer Protocol
        { "SRP", 119 }, // SpectraLink Radio Protocol
        { "UTI", 120 }, // Universal Transport Interface Protocol
        { "SMP", 121 }, // Simple Message Protocol
        { "SM", 122 }, // Simple Multicast Protocol
        { "PTP", 123 }, // Performance Transparency Protocol
        { "IS-IS-IPV4", 124 }, // Intermediate System to Intermediate System Protocol over IPv4
        { "FIRE", 125 }, // Flexible Intra-AS Routing Environment
        { "CRTP", 126 }, // Combat Radio Transport Protocol
        { "CRUDP", 127 }, // Combat Radio User Datagram
        { "SSCOPMCE", 128 }, // Service-Specific Connection-Oriented Protocol in a Multilink and
                             // Connectionless Environment
        { "IPLT", 129 }, // IPLT
        { "SPS", 130 }, // Secure Packet Shield
        { "PIPE", 131 }, // Private IP Encapsulation within IP
        { "SCTP", 132 }, // Stream Control Transmission Protocol
        { "FC", 133 }, // Fibre Channel
        { "RSVP-E2E-IGNORE", 134 }, // Reservation Protocol (RSVP) End-to-End Ignore
        { "MOBILITY-HEADER", 135 }, // Mobility Extension Header for IPv6
        { "UDPLITE", 136 }, // Lightweight User Datagram Protocol
        { "MPLS-IN-IP", 137 }, // Multiprotocol Label Switching Encapsulated in IP
        { "MANET", 138 }, // MANET Protocols
        { "HIP", 139 }, // Host Identity Protocol
        { "SHIM6", 140 }, // Site Multihoming by IPv6 Intermediation
        { "WESP", 141 }, // Wrapped Encapsulating Security Payload
        { "ROHC", 142 }, // Robust Header Compression
        { "ETHERNET", 143 }, // Segment Routing over IPv6
        { "AGGFRAG", 144 }, // AGGFRAG Encapsulation Payload for ESP
        { "NSH", 145 }, // Network Service Header
        { "RAWSOCKET", 255 }, // Raw socket
    };

    const auto nameUpper = name.toString().toUpper();

    return protocolNameNumbersMap.value(nameUpper, 0);
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
