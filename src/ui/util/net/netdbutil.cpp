#include "netdbutil.h"

#include <sqlite/sqlitestmt.h>

#include "netutil.h"

QVariant NetDbUtil::ip4Var(const ip_addr_t &ip, bool isIPv6)
{
    return isIPv6 ? QVariant() : QVariant(qint32(ip.v4));
}

QVariant NetDbUtil::ip6Var(const ip_addr_t &ip, bool isIPv6)
{
    return isIPv6 ? QVariant(NetUtil::ip6ToArrayView(ip.v6).toByteArray()) : QVariant();
}

bool NetDbUtil::columnIsIPv6(const SqliteStmt &stmt, int column4)
{
    return stmt.columnIsNull(column4);
}

ip_addr_t NetDbUtil::columnIp(const SqliteStmt &stmt, int column4, int column6, bool isIPv6)
{
    ip_addr_t ip = {};

    if (isIPv6) {
        ip.v6 = NetUtil::arrayViewToIp6(stmt.columnBlob(column6, /*isView=*/true));
    } else {
        ip.v4 = stmt.columnUInt(column4);
    }

    return ip;
}
