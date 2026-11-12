#include "dnsquery.h"

#include <ares.h>

#include "dnsutil.h"

namespace {

inline constexpr quint16 udpSizeMin = 512; // without EDNS

quint8 checkErrorRcode(const ares_dns_record_t *record)
{
    if (ares_dns_record_get_opcode(record) != ARES_OPCODE_QUERY)
        return ARES_RCODE_NOTIMP;

    return ARES_RCODE_NOERROR;
}

}

bool DnsQuery::parse(const QByteArray &data)
{
    m_record = DnsUtil::parseRecord(data);
    if (!m_record)
        return false;

    const ares_dns_record_t *record = m_record.get();

    // A response
    if ((ares_dns_record_get_flags(record) & ARES_FLAG_QR) != 0)
        return false;

    m_id = ares_dns_record_get_id(record);
    m_errorRcode = checkErrorRcode(record);

    parseOpt();
    parseName();

    return true;
}

void DnsQuery::parseOpt()
{
    const int optIndex = DnsUtil::optIndex(record());

    m_hasOpt = (optIndex >= 0);
    m_udpSizeMax = udpSizeMin;

    if (m_hasOpt) {
        const ares_dns_rr_t *rr =
                ares_dns_record_rr_get_const(record(), ARES_SECTION_ADDITIONAL, optIndex);

        m_udpSizeMax = qMax(m_udpSizeMax, ares_dns_rr_get_u16(rr, ARES_RR_OPT_UDP_SIZE));
    }
}

void DnsQuery::parseName()
{
    const char *name = nullptr;

    if (ares_dns_record_query_get(record(), 0, &name, nullptr, nullptr) == ARES_SUCCESS) {
        m_name = QString::fromLatin1(name);
    } else {
        m_name.clear();
    }
}
