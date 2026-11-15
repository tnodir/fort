#ifndef DNSPROVIDER_H
#define DNSPROVIDER_H

#include <QList>
#include <QString>

// A DNS provider's upstream servers to choose in the options
struct DnsProvider
{
    // The providers of ":/dns/data/providers.json"
    static QList<DnsProvider> readList();

    QString title;
    QString servers; // comma-separated
};

#endif // DNSPROVIDER_H
