#ifndef DNSUTIL_H
#define DNSUTIL_H

#include <QByteArray>

#include <memory>

struct ares_dns_record;

using DnsRecordPtr = std::unique_ptr<ares_dns_record, void (*)(ares_dns_record *)>;

class DnsUtil
{
public:
    // The index of the OPT record in the Additional section, or -1
    static int optIndex(const ares_dns_record *record);

    // Null for a wrong message, e.g. without a single question
    static DnsRecordPtr parseRecord(const QByteArray &data);

    // The record's copy with the id
    static DnsRecordPtr duplicateRecord(const ares_dns_record *record, quint16 id);

    static QByteArray writeRecord(const ares_dns_record *record);
};

#endif // DNSUTIL_H
