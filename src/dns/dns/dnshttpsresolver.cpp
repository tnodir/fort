#include "dnshttpsresolver.h"

#include <QLoggingCategory>
#include <QNetworkReply>

#include <algorithm>
#include <numeric>

#include <ares.h>

struct DnsHttpsQuery
{
    DnsRequest request;
    QByteArray data;
    QList<int> serverIndexes; // of the servers to ask yet
};

namespace {

const QLoggingCategory LC("dns.httpsResolver");

inline constexpr char dnsMessageType[] = "application/dns-message";

inline constexpr int requestTimeout = 4 * 1000; // msecs
inline constexpr int serverFailedTime = 5 * 60 * 1000; // msecs

QNetworkRequest createRequest(const DnsHttpsServer &server)
{
    QNetworkRequest request(server.url());

    request.setPeerVerifyName(server.verifyName());
    request.setHeader(QNetworkRequest::ContentTypeHeader, dnsMessageType);
    request.setRawHeader("Accept", dnsMessageType);
    request.setTransferTimeout(requestTimeout);

    // A redirect's host name would be resolved by the system's resolver, i.e. by the forwarder
    request.setAttribute(
            QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    // The server doesn't track the client by cookies
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);

    return request;
}

}

DnsHttpsResolver::DnsHttpsResolver(QObject *parent) : DnsResolver(parent) { }

void DnsHttpsResolver::setServers(const QList<DnsHttpsServer> &servers)
{
    m_servers = servers;
    m_failedTimers = QList<QDeadlineTimer>(servers.size());
}

bool DnsHttpsResolver::open(const QString &servers)
{
    setServers(DnsHttpsServer::parseServers(servers));

    if (m_servers.isEmpty()) {
        qCWarning(LC) << "Servers error:" << servers;
        return false;
    }

    return true;
}

void DnsHttpsResolver::resolve(const DnsRequest &request)
{
    // Without the id, for the HTTP caches
    const DnsHttpsQueryPtr query(
            new DnsHttpsQuery { request, request.query.write(/*id=*/0), serverIndexes() });

    sendQuery(query);
}

QList<int> DnsHttpsResolver::serverIndexes() const
{
    QList<int> indexes(m_servers.size());

    std::iota(indexes.begin(), indexes.end(), 0);

    std::stable_partition(indexes.begin(), indexes.end(),
            [&](int serverIndex) { return !isServerFailed(serverIndex); });

    return indexes;
}

bool DnsHttpsResolver::isServerFailed(int serverIndex) const
{
    return !m_failedTimers.at(serverIndex).hasExpired();
}

void DnsHttpsResolver::setServerFailed(int serverIndex, bool failed)
{
    m_failedTimers[serverIndex] = failed ? QDeadlineTimer(serverFailedTime) : QDeadlineTimer();
}

void DnsHttpsResolver::sendQuery(const DnsHttpsQueryPtr &query)
{
    if (query->serverIndexes.isEmpty()) {
        DnsAnswer answer;
        answer.setRcode(query->request.query, ARES_RCODE_SERVFAIL);

        emit resolved(query->request, answer);
        return;
    }

    const int serverIndex = query->serverIndexes.takeFirst();

    QNetworkReply *reply = m_manager.post(createRequest(m_servers.at(serverIndex)), query->data);

    connect(reply, &QNetworkReply::finished, this,
            [=, this] { processReply(query, serverIndex, reply); });
}

void DnsHttpsResolver::processReply(
        const DnsHttpsQueryPtr &query, int serverIndex, QNetworkReply *reply)
{
    reply->deleteLater();

    const DnsRequest &request = query->request;

    DnsAnswer answer;
    const bool ok = (reply->error() == QNetworkReply::NoError)
            && answer.setData(request.query, reply->readAll(), request.responseSizeMax());

    if (!ok && !isServerFailed(serverIndex)) {
        qCWarning(LC) << "Server error:" << reply->url().host() << reply->errorString();
    }

    setServerFailed(serverIndex, !ok);

    if (ok) {
        emit resolved(request, answer);
    } else {
        sendQuery(query);
    }
}
