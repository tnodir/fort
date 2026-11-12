#ifndef DNSREQUEST_H
#define DNSREQUEST_H

#include <QAbstractSocket>
#include <QHostAddress>
#include <QPointer>

#include "dnsquery.h"

// The client to send the response to
struct DnsClient
{
    bool tcp = false;
    quint16 port = 0;
    QHostAddress address;
    QPointer<QAbstractSocket> socket;
};

struct DnsRequest
{
    int responseSizeMax() const { return client.tcp ? 0xFFFF : query.udpSizeMax(); }

    DnsClient client;
    DnsQuery query;
};

#endif // DNSREQUEST_H
