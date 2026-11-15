#ifndef DNSANSWER_H
#define DNSANSWER_H

#include <QByteArray>

#include "dnsaddresses.h"

struct ares_dns_record;
struct ares_dns_rr;

class DnsQuery;

class DnsAnswer
{
public:
    const QByteArray &response() const { return m_response; }

    const DnsAddresses &addresses() const { return m_addresses; }

    // The upstream server's answer to the query
    void setRecord(const DnsQuery &query, const ares_dns_record *record, int sizeMax);

    // The own answer to the query, without records
    void setRcode(const DnsQuery &query, quint8 rcode, quint16 flags = 0);

private:
    void readAddresses(const ares_dns_record *record);
    bool readAddress(const ares_dns_rr *rr);

private:
    QByteArray m_response;

    DnsAddresses m_addresses;
};

#endif // DNSANSWER_H
