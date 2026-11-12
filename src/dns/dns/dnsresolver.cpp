#include "dnsresolver.h"

#include "dnshttpsresolver.h"
#include "dnsplainresolver.h"

DnsResolver::DnsResolver(QObject *parent) : QObject(parent) { }

std::unique_ptr<DnsResolver> DnsResolver::create(const QString &servers)
{
    if (DnsHttpsServer::isHttpsServers(servers)) {
        return std::make_unique<DnsHttpsResolver>();
    }

    return std::make_unique<DnsPlainResolver>();
}
