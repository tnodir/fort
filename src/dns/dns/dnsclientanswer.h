#ifndef DNSCLIENTANSWER_H
#define DNSCLIENTANSWER_H

#include <QString>

#include "dnsaddresses.h"

// The DNS Client's answer to a process's query, of its ETW event
class DnsClientAnswer
{
public:
    quint32 processId() const { return m_processId; }
    void setProcessId(quint32 v) { m_processId = v; }

    const QString &name() const { return m_name; }
    void setName(const QString &v) { m_name = v; }

    const DnsAddresses &addresses() const { return m_addresses; }

    // The event's QueryResults, e.g. "type: 5 alias.com;::ffff:1.2.3.4;2001:db8::1;"
    void readQueryResults(QStringView results);

private:
    void readAddress(QStringView text);

private:
    quint32 m_processId = 0;

    QString m_name;

    DnsAddresses m_addresses;
};

#endif // DNSCLIENTANSWER_H
