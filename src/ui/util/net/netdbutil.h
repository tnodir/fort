#ifndef NETDBUTIL_H
#define NETDBUTIL_H

#include <common/common_types.h>

class SqliteStmt;

// A DB row's IP address in its IPv4 and IPv6 columns: the IPv4 one is NULL for an IPv6 one
class NetDbUtil
{
public:
    static bool columnIsIPv6(const SqliteStmt &stmt, int column4);
    static ip_addr_t columnIp(const SqliteStmt &stmt, int column4, int column6, bool isIPv6);
};

#endif // NETDBUTIL_H
