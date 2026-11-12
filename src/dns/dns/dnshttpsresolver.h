#ifndef DNSHTTPSRESOLVER_H
#define DNSHTTPSRESOLVER_H

#include <QDeadlineTimer>
#include <QNetworkAccessManager>

#include "dnshttpsserver.h"
#include "dnsresolver.h"

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

struct DnsHttpsQuery;

// Sends the queries to the upstream servers by DNS over HTTPS (RFC 8484): to the first server,
// to the next ones when it fails
class DnsHttpsResolver : public DnsResolver
{
    Q_OBJECT

public:
    explicit DnsHttpsResolver(QObject *parent = nullptr);

    const QList<DnsHttpsServer> &servers() const { return m_servers; }
    void setServers(const QList<DnsHttpsServer> &servers);

    // The comma-separated URLs
    bool open(const QString &servers) override;

    void resolve(const DnsRequest &request) override;

private:
    using DnsHttpsQueryPtr = std::shared_ptr<DnsHttpsQuery>;

    // The servers to ask in turn: the failed ones are the last
    QList<int> serverIndexes() const;

    bool isServerFailed(int serverIndex) const;
    void setServerFailed(int serverIndex, bool failed);

    void sendQuery(const DnsHttpsQueryPtr &query);
    void processReply(const DnsHttpsQueryPtr &query, int serverIndex, QNetworkReply *reply);

private:
    QList<DnsHttpsServer> m_servers;

    // The servers' times to ask them after the other ones till
    QList<QDeadlineTimer> m_failedTimers;

    QNetworkAccessManager m_manager;
};

#endif // DNSHTTPSRESOLVER_H
