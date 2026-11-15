#ifndef DNSQUERY_H
#define DNSQUERY_H

#include <QByteArray>
#include <QString>

#include <memory>

struct ares_dns_record;

class DnsQuery
{
public:
    bool hasOpt() const { return m_hasOpt; }

    // The response code of an unsupported query, else 0
    quint8 errorRcode() const { return m_errorRcode; }

    quint16 id() const { return m_id; }

    // The client's max size of a UDP response
    quint16 udpSizeMax() const { return m_udpSizeMax; }

    // The question's name
    const QString &name() const { return m_name; }

    const ares_dns_record *record() const { return m_record.get(); }

    bool parse(const QByteArray &data);

private:
    void parseOpt();
    void parseName();

private:
    bool m_hasOpt = false;
    quint8 m_errorRcode = 0;
    quint16 m_id = 0;
    quint16 m_udpSizeMax = 0;
    QString m_name;
    std::shared_ptr<ares_dns_record> m_record;
};

#endif // DNSQUERY_H
