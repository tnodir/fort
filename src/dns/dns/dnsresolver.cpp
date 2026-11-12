#include "dnsresolver.h"

#include "dnsplainresolver.h"

DnsResolver::DnsResolver(QObject *parent) : QObject(parent) { }

std::unique_ptr<DnsResolver> DnsResolver::create(const QString & /*servers*/)
{
    return std::make_unique<DnsPlainResolver>();
}
