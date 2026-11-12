#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <QHash>
#include <QObject>

#include "dnsanswer.h"
#include "dnsrequest.h"

struct ares_channeldata;

struct DnsResolveContext;

// Sends the queries to the upstream servers by a c-ares channel, on its event thread
class DnsResolver : public QObject
{
    Q_OBJECT

public:
    explicit DnsResolver(QObject *parent = nullptr);
    ~DnsResolver() override;
    Q_DISABLE_COPY_MOVE(DnsResolver)

    bool isOpened() const { return m_channel != nullptr; }

    // The comma-separated servers, empty for the adapters' DNS servers
    bool open(const QString &servers);
    void close();

    // Whether the query is the resolver's own one, sent to the listener: a loop
    bool isOwnQuery(const DnsQuery &query) const;

    void resolve(const DnsRequest &request);

signals:
    void resolved(const DnsRequest &request, const DnsAnswer &answer);

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

#endif // DNSRESOLVER_H
