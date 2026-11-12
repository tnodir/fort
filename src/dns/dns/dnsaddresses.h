#ifndef DNSADDRESSES_H
#define DNSADDRESSES_H

#include <QVector>

#include <cstring>

#include <common/common_types.h>

// The addresses of an answer with their max TTL: an RRset's records have the same TTL (RFC 2181),
// and an answer's addresses are usually of one RRset, of the last CNAME target
class DnsAddresses
{
public:
    bool isEmpty() const { return m_ip4Array.isEmpty() && m_ip6Array.isEmpty(); }

    quint32 ttl() const { return m_ttl; }
    void addTtl(quint32 ttl) { m_ttl = qMax(m_ttl, ttl); }

    const QVector<quint32> &ip4Array() const { return m_ip4Array; }
    const QVector<ip6_addr_t> &ip6Array() const { return m_ip6Array; }

    void addIp4(quint32 ip) { m_ip4Array.append(ip); }
    void addIp6(const quint8 *bytes)
    {
        ip6_addr_t ip;
        std::memcpy(ip.data, bytes, sizeof(ip.data));

        m_ip6Array.append(ip);
    }

private:
    quint32 m_ttl = 0;

    QVector<quint32> m_ip4Array;
    QVector<ip6_addr_t> m_ip6Array;
};

#endif // DNSADDRESSES_H
