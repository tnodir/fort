#include "dnsprovider.h"

#include <QLoggingCategory>
#include <QVariant>

#include <util/json/jsonutil.h>

namespace {

const QLoggingCategory LC("dns.provider");

}

QList<DnsProvider> DnsProvider::readList()
{
    QString errorString;
    const auto items = JsonUtil::fileToVariant(":/dns/data/providers.json", errorString).toList();
    if (!errorString.isEmpty()) {
        qCWarning(LC) << "DNS Providers: JSON error:" << errorString;
    }

    QList<DnsProvider> list;

    for (const auto &itemVar : items) {
        const QVariantMap map = itemVar.toMap();

        list.append(DnsProvider {
                .title = map["title"].toString(),
                .servers = map["servers"].toStringList().join(", "),
        });
    }

    return list;
}
