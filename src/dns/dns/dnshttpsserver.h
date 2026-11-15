#ifndef DNSHTTPSSERVER_H
#define DNSHTTPSSERVER_H

#include <QList>
#include <QUrl>

// A DNS over HTTPS server (RFC 8484) of the URL with the server's address: as its host, or as the
// host name's "ip" parameter, e.g. "https://dns.example/dns-query?ip=192.0.2.1"
class DnsHttpsServer
{
public:
    DnsHttpsServer() = default;
    explicit DnsHttpsServer(const QUrl &url, const QString &verifyName = {});

    // The URL to request: by the server's address
    const QUrl &url() const { return m_url; }

    // The host name to verify the server's certificate by, empty to verify by the address
    const QString &verifyName() const { return m_verifyName; }

    bool parse(const QString &text);

    // Whether the server's text is of a DNS over HTTPS one, valid or not
    static bool isHttpsServer(const QString &text);

    // Whether the list has both the DNS over HTTPS and plain servers: a resolver is of one protocol
    static bool isMixedServers(const QStringList &servers);

    // Empty with a wrong server
    static QList<DnsHttpsServer> parseServers(const QStringList &servers);

private:
    QUrl m_url;
    QString m_verifyName;
};

#endif // DNSHTTPSSERVER_H
