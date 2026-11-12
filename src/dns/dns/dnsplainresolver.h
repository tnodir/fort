#ifndef DNSPLAINRESOLVER_H
#define DNSPLAINRESOLVER_H

#include <QHash>

#include "dnsresolver.h"

struct ares_channeldata;

struct DnsResolveContext;

// Sends the queries to the upstream servers by UDP and TCP: a c-ares channel, on its event thread
class DnsPlainResolver : public DnsResolver
{
    Q_OBJECT

public:
    explicit DnsPlainResolver(QObject *parent = nullptr);
    ~DnsPlainResolver() override;
    Q_DISABLE_COPY_MOVE(DnsPlainResolver)

    bool isOpened() const { return m_channel != nullptr; }

    // The comma-separated addresses, empty for the adapters' DNS servers
    bool open(const QString &servers) override;
    void close();

    bool isOwnQuery(const DnsQuery &query) const override;

    void resolve(const DnsRequest &request) override;

private:
    bool initChannel();
    bool setServers(const QString &servers);

    void removePendingQuery(const DnsResolveContext *context);

    // Called on the c-ares thread
    static void onResolved(void *arg, int status, const ares_dns_record *record);

private:
    ares_channeldata *m_channel = nullptr;

    // The sent queries by their ids
    QHash<quint16, const DnsResolveContext *> m_pendingQueries;
};

#endif // DNSPLAINRESOLVER_H
