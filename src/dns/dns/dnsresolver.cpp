#include "dnsresolver.h"

#include "dnshttpsresolver.h"
#include "dnsplainresolver.h"

DnsResolver::DnsResolver(QObject *parent) : QObject(parent) { }

std::unique_ptr<DnsResolver> DnsResolver::create(const QStringList &servers)
{
    if (DnsHttpsServer::isMixedServers(servers))
        return nullptr;

    if (DnsHttpsServer::isHttpsServer(servers.value(0))) {
        return std::make_unique<DnsHttpsResolver>();
    }

    return std::make_unique<DnsPlainResolver>();
}
