#ifndef DNSANSWER_H
#define DNSANSWER_H

#include <QByteArray>
#include <QString>
#include <QStringList>

#include "dnsaddresses.h"

struct ares_dns_record;
struct ares_dns_rr;

class DnsQuery;

class DnsAnswer
{
public:
    // The response code of the upstream server's answer or of the own one
    quint8 rcode() const { return m_rcode; }

    const QByteArray &response() const { return m_response; }

    const DnsAddresses &addresses() const { return m_addresses; }

    // The CNAME records' targets, the canonical names of the answer
    const QStringList &cnameTargets() const { return m_cnameTargets; }

    // The resolver's error, empty for the upstream server's answer
    const QString &error() const { return m_error; }

    // The upstream server's answer to the query
    void setRecord(const DnsQuery &query, const ares_dns_record *record, int sizeMax);

    // The upstream server's answer to the query, as received: false for a wrong message or
    // another question's answer
    bool setData(const DnsQuery &query, const QByteArray &data, int sizeMax);

    // The own answer to the query, without records
    void setRcode(const DnsQuery &query, quint8 rcode, quint16 flags = 0);

    // The own SERVFAIL answer to the query the resolver failed to send or to get the answer of
    void setError(const DnsQuery &query, const QString &error);

private:
    void readAnswers(const ares_dns_record *record);
    bool readAddress(const ares_dns_rr *rr);

private:
    quint8 m_rcode = 0;

    QByteArray m_response;

    DnsAddresses m_addresses;

    QStringList m_cnameTargets;

    QString m_error;
};

#endif // DNSANSWER_H
