#include "dnshttpsserver.h"

#include <QHostAddress>
#include <QUrlQuery>

namespace {

inline constexpr char ipKey[] = "ip";

bool isAddress(const QString &host)
{
    return !QHostAddress(host).isNull();
}

}

DnsHttpsServer::DnsHttpsServer(const QUrl &url, const QString &verifyName) :
    m_url(url), m_verifyName(verifyName)
{
}

bool DnsHttpsServer::parse(const QString &text)
{
    QUrl url(text.trimmed(), QUrl::StrictMode);
    if (!url.isValid() || url.scheme() != QLatin1String("https"))
        return false;

    QUrlQuery query(url);
    const QString ip = query.queryItemValue(ipKey);
    QString verifyName;

    // The host name isn't resolved: the system's resolver asks the forwarder itself
    if (!ip.isEmpty()) {
        verifyName = url.host();

        query.removeQueryItem(ipKey);
        url.setQuery(query);
        url.setHost(ip);
    }

    if (!isAddress(url.host()) || isAddress(verifyName))
        return false;

    m_url = url;
    m_verifyName = verifyName;

    return true;
}

bool DnsHttpsServer::isHttpsServers(const QString &servers)
{
    return servers.trimmed().startsWith(QLatin1String("https://"), Qt::CaseInsensitive);
}

QList<DnsHttpsServer> DnsHttpsServer::parseServers(const QString &servers)
{
    QList<DnsHttpsServer> list;

    const auto texts = servers.simplified().split(u',', Qt::SkipEmptyParts);

    for (const QString &text : texts) {
        DnsHttpsServer server;
        if (!server.parse(text))
            return {};

        list.append(server);
    }

    return list;
}
