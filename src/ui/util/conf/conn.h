#ifndef CONN_H
#define CONN_H

#include <QDateTime>
#include <QString>

#include <common/common_types.h>

// A collected connection
class Conn
{
public:
    bool isIPv6 : 1 = false;
    bool blocked : 1 = false;
    bool alerted : 1 = false;
    bool inherited : 1 = false;
    bool inbound : 1 = false;
    bool loopback : 1 = false;

    quint8 reason = 0;

    quint8 ipProto = 0;
    quint8 zoneId = 0;
    quint16 ruleId = 0;
    quint16 localPort = 0;
    quint16 remotePort = 0;

    quint32 pid = 0;

    ip_addr_t localIp;
    ip_addr_t remoteIp;

    QString appPath;
    QString inheritAppPath;

    QDateTime connTime;
};

#endif // CONN_H
