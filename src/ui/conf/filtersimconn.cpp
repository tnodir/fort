#include "filtersimconn.h"

#include <QUrl>

#include <common/fortdef.h>

#include <util/conf/conn.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>

void FilterSimConn::setConn(const Conn &v)
{
    appPath = v.appPath;
    hostName = v.remoteHost;

    conn.inbound = v.inbound;
    conn.isIPv6 = v.isIPv6;
    conn.is_loopback = v.loopback;
    // The Service's connection is known by its reason only
    conn.is_fort_service = (v.reason == FORT_CONN_REASON_DNS_FORWARDER);
    conn.ip_proto = v.ipProto;
    conn.local_port = v.localPort;
    conn.remote_port = v.remotePort;
    conn.local_ip = v.localIp;
    conn.remote_ip = v.remoteIp;
}

bool FilterSimConn::setRemoteIp(const QString &text, bool isLoopback)
{
    bool isIPv6 = false;

    if (!NetFormatUtil::textToIp(text, conn.remote_ip, isIPv6))
        return false;

    conn.isIPv6 = isIPv6;
    conn.is_loopback = isLoopback || NetUtil::isLoopbackIp(conn.remote_ip, isIPv6);

    return true;
}

bool FilterSimConn::setLocalIp(const QString &text)
{
    const QString ipText = text.trimmed();
    if (ipText.isEmpty())
        return true;

    bool isIPv6 = false;

    return NetFormatUtil::textToIp(ipText, conn.local_ip, isIPv6) && isIPv6 == bool(conn.isIPv6);
}

bool FilterSimConn::setHostName(const QString &text)
{
    const QString name = text.trimmed();
    if (name.isEmpty()) {
        hostName.clear();
        return true;
    }

    hostName = QString::fromLatin1(QUrl::toAce(name));

    return !hostName.isEmpty();
}
