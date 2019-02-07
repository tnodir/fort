#include "netutil.h"

#include <QLocale>

#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>

struct sock_addr {
  union {
    struct sockaddr addr;
    struct sockaddr_in in;
  } u;
  int addrlen;
};

#define MAX_IPV4_LEN    20
#define SOCK_ADDR_LEN   offsetof(struct sock_addr, addrlen)

#define sock_addr_get_inp(sap) \
    ((void *) &(sap)->u.in.sin_addr)

NetUtil::NetUtil(QObject *parent) :
    QObject(parent)
{
}

quint32 NetUtil::textToIp4(const QString &text, bool *ok)
{
    quint32 ip4 = 0;
    void *p = &ip4;

    const bool res = InetPtonW(AF_INET, (PCWSTR) text.utf16(), p) == 1;

    if (ok) {
        *ok = res;
    }

    return ntohl(*((unsigned long *) p));
}

QString NetUtil::ip4ToText(quint32 ip)
{
    quint32 ip4 = htonl((unsigned long) ip);
    const void *p = &ip4;
    wchar_t buf[MAX_IPV4_LEN];

    if (!InetNtopW(AF_INET, (PVOID) p, buf, MAX_IPV4_LEN))
        return QString();

    return QString::fromWCharArray(buf);
}

int NetUtil::ip4Mask(quint32 ip)
{
    return 32 - first0Bit(ip);
}

int NetUtil::first0Bit(quint32 u)
{
    const qint32 i = ~u;
    return bitCount((i & (-i)) - 1);
}

// From http://tekpool.wordpress.com/category/bit-count/
int NetUtil::bitCount(quint32 u)
{
    const quint32 uCount = u
            - ((u >> 1) & 033333333333)
            - ((u >> 2) & 011111111111);

    return ((uCount + (uCount >> 3))
            & 030707070707) % 63;
}

QString NetUtil::formatDataSize(qint64 bytes, int precision)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    return QString::number(double(bytes), 'f', precision);
#else
    return QLocale::c().formattedDataSize(bytes, precision);
#endif
}

QString NetUtil::formatSpeed(quint32 bytes)
{
    QString text = formatDataSize(bytes, 1);
    text.remove(QLatin1String(".0"));
    return text + QLatin1String("/s");
}

QString NetUtil::getHostName(const QString &address)
{
    WCHAR hostName[NI_MAXHOST];

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(textToIp4(address));

    if (GetNameInfoW((struct sockaddr *) &sa, sizeof(struct sockaddr),
                     hostName, NI_MAXHOST, nullptr, 0, 0))
        return QString();

    return QString::fromWCharArray(hostName);
}

QStringList NetUtil::localIpv4Networks()
{
    static QStringList list = QStringList()
            << "0.0.0.0/32"  // non-routable meta-address
            << "10.0.0.0/8"
            << "100.64.0.0/10"  // for carrier-grade NAT deployment
            << "127.0.0.0/8"  // Loopback
            << "169.254.0.0/16"  // if cannot obtain a network address via DHCP
            << "172.16.0.0/12"
            << "192.168.0.0/16"
            << "239.255.255.250/32"  // IP Multicast for DLNA/UPNP
            << "255.255.255.255/32"  // IP Broadcast
               ;

    return list;
}
