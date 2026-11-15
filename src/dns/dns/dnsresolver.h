#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <QObject>

#include <memory>

#include "dnsanswer.h"
#include "dnsrequest.h"

// Sends the queries to the upstream servers
class DnsResolver : public QObject
{
    Q_OBJECT

public:
    explicit DnsResolver(QObject *parent = nullptr);

    // The servers as of DnsUtil::splitServers()
    virtual bool open(const QStringList &servers) = 0;

    // Whether the query is the resolver's own one, sent to the listener: a loop
    virtual bool isOwnQuery(const DnsQuery & /*query*/) const { return false; }

    virtual void resolve(const DnsRequest &request) = 0;

    // The resolver of the servers' protocol
    static std::unique_ptr<DnsResolver> create(const QStringList &servers);

signals:
    void resolved(const DnsRequest &request, const DnsAnswer &answer);
};

#endif // DNSRESOLVER_H
