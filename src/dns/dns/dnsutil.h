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

    // The messages of the single question, as parsed by c-ares: the same name, type and class
    static bool isSameQuestion(const ares_dns_record *record, const ares_dns_record *other);

    // Null for a wrong message, e.g. without a single question
    static DnsRecordPtr parseRecord(const QByteArray &data);

    // The record's copy with the id
    static DnsRecordPtr duplicateRecord(const ares_dns_record *record, quint16 id);

    static QByteArray writeRecord(const ares_dns_record *record);

    // Clears the Dnscache's answers
    static bool flushResolverCache();
};

#endif // DNSUTIL_H
