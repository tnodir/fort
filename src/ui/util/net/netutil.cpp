#include "netutil.h"

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
        return false;

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
