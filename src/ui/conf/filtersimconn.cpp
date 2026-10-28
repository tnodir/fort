#include "filtersimconn.h"

#include <util/net/netformatutil.h>
#include <util/net/netutil.h>

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
