#include "dnsanswer.h"

#include <QtEndian>

#include <ares.h>

#include "dnsquery.h"
#include "dnsutil.h"

namespace {

inline constexpr quint16 ednsUdpSize = 1232; // of the DNS Flag Day 2020
inline constexpr quint32 negativeTtl = 10; // seconds, of the NXDOMAIN answer

void copyQuestions(const ares_dns_record_t *query, ares_dns_record_t *record)
{
    const size_t count = ares_dns_record_query_cnt(query);

    for (size_t i = 0; i < count; ++i) {
        const char *name = nullptr;
        ares_dns_rec_type_t type;
        ares_dns_class_t qclass;

        ares_dns_record_query_get(query, i, &name, &type, &qclass);
        ares_dns_record_query_add(record, name, type, qclass);
    }
}

// The root zone's SOA record to cache the negative answer by
void addSoa(ares_dns_record_t *record)
{
    ares_dns_rr_t *rr = nullptr;

    if (ares_dns_record_rr_add(&rr, record, ARES_SECTION_AUTHORITY, "", ARES_REC_TYPE_SOA,
                ARES_CLASS_IN, negativeTtl)
            != ARES_SUCCESS)
        return;

    ares_dns_rr_set_str(rr, ARES_RR_SOA_MNAME, "localhost");
    ares_dns_rr_set_str(rr, ARES_RR_SOA_RNAME, "localhost");
    ares_dns_rr_set_u32(rr, ARES_RR_SOA_MINIMUM, negativeTtl);
}

void addOpt(ares_dns_record_t *record)
{
    ares_dns_rr_t *rr = nullptr;

    if (ares_dns_record_rr_add(
                &rr, record, ARES_SECTION_ADDITIONAL, "", ARES_REC_TYPE_OPT, ARES_CLASS_IN, 0)
            != ARES_SUCCESS)
        return;

    ares_dns_rr_set_u16(rr, ARES_RR_OPT_UDP_SIZE, ednsUdpSize);
    ares_dns_rr_set_u8(rr, ARES_RR_OPT_VERSION, 0);
    ares_dns_rr_set_u16(rr, ARES_RR_OPT_FLAGS, 0);
}

QByteArray createResponse(const DnsQuery &query, quint8 rcode, quint16 flags)
{
    const ares_dns_record_t *queryRecord = query.record();

    flags |= ARES_FLAG_QR | ARES_FLAG_RA
            | (ares_dns_record_get_flags(queryRecord) & (ARES_FLAG_RD | ARES_FLAG_CD));

    ares_dns_record_t *record = nullptr;

    if (ares_dns_record_create(&record, query.id(), flags, ares_dns_record_get_opcode(queryRecord),
                ares_dns_rcode_t(rcode))
            != ARES_SUCCESS)
        return {};

    const DnsRecordPtr recordPtr(record, &ares_dns_record_destroy);

    copyQuestions(queryRecord, record);

    if (rcode == ARES_RCODE_NXDOMAIN) {
        addSoa(record);
    }

    if (query.hasOpt()) {
        addOpt(record);
    }

    return DnsUtil::writeRecord(record);
}

// The upstream's EDNS isn't the client's one: its flags and options stay, but the cookies
void fixOpt(const DnsQuery &query, ares_dns_record_t *record)
{
    const int optIndex = DnsUtil::optIndex(record);
    if (optIndex < 0)
        return;

    if (!query.hasOpt()) {
        ares_dns_record_rr_del(record, ARES_SECTION_ADDITIONAL, optIndex);
        return;
    }

    ares_dns_rr_t *rr = ares_dns_record_rr_get(record, ARES_SECTION_ADDITIONAL, optIndex);

    ares_dns_rr_set_u16(rr, ARES_RR_OPT_UDP_SIZE, ednsUdpSize);
    ares_dns_rr_del_opt_byid(rr, ARES_RR_OPT_OPTIONS, ARES_OPT_PARAM_COOKIE);
}

bool isQueryResponse(const DnsQuery &query, const ares_dns_record_t *record)
{
    if ((ares_dns_record_get_flags(record) & ARES_FLAG_QR) == 0)
        return false;

    return DnsUtil::isSameQuestion(query.record(), record);
}

QByteArray forwardResponse(const DnsQuery &query, const ares_dns_record_t *answer)
{
    const DnsRecordPtr record = DnsUtil::duplicateRecord(answer, query.id());
    if (!record)
        return {};

    fixOpt(query, record.get());

    return DnsUtil::writeRecord(record.get());
}

// The value's size of the SVCB or HTTPS record's parameter, 0 when absent
size_t paramValue(const ares_dns_rr_t *rr, ares_dns_rr_key_t key, ares_svcb_param_t param,
        const unsigned char **val)
{
    size_t len = 0;

    if (!ares_dns_rr_get_opt_byid(rr, key, param, val, &len))
        return 0;

    return len;
}

// The ipv4hint and ipv6hint addresses of the SVCB or HTTPS record (RFC 9460)
bool readHints(DnsAddresses &addresses, const ares_dns_rr_t *rr, ares_dns_rr_key_t key)
{
    const unsigned char *ip4 = nullptr;
    const size_t ip4Count = paramValue(rr, key, ARES_SVCB_PARAM_IPV4HINT, &ip4) / sizeof(quint32);

    for (size_t i = 0; i < ip4Count; ++i) {
        addresses.addIp4(qFromBigEndian<quint32>(ip4 + i * sizeof(quint32)));
    }

    const unsigned char *ip6 = nullptr;
    const size_t ip6Count =
            paramValue(rr, key, ARES_SVCB_PARAM_IPV6HINT, &ip6) / sizeof(ip6_addr_t);

    for (size_t i = 0; i < ip6Count; ++i) {
        addresses.addIp6(ip6 + i * sizeof(ip6_addr_t));
    }

    return ip4Count != 0 || ip6Count != 0;
}

// The target name of the CNAME, SVCB or HTTPS record, else null
const char *targetName(const ares_dns_rr_t *rr)
{
    switch (ares_dns_rr_get_type(rr)) {
    case ARES_REC_TYPE_CNAME:
        return ares_dns_rr_get_str(rr, ARES_RR_CNAME_CNAME);
    case ARES_REC_TYPE_SVCB:
        return ares_dns_rr_get_str(rr, ARES_RR_SVCB_TARGET);
    case ARES_REC_TYPE_HTTPS:
        return ares_dns_rr_get_str(rr, ARES_RR_HTTPS_TARGET);
    default:
        return nullptr;
    }
}

}

void DnsAnswer::setRecord(const DnsQuery &query, const ares_dns_record *record, int sizeMax)
{
    readAnswers(record);

    m_rcode = quint8(ares_dns_record_get_rcode(record));
    m_response = forwardResponse(query, record);

    if (m_response.isEmpty()) {
        setRcode(query, ARES_RCODE_SERVFAIL);
    } else if (m_response.size() > sizeMax) {
        setRcode(query, ARES_RCODE_NOERROR, ARES_FLAG_TC);
    }
}

bool DnsAnswer::setData(const DnsQuery &query, const QByteArray &data, int sizeMax)
{
    const DnsRecordPtr record = DnsUtil::parseRecord(data);

    // Not a response to the query
    if (!record || !isQueryResponse(query, record.get()))
        return false;

    setRecord(query, record.get(), sizeMax);

    return true;
}

void DnsAnswer::setRcode(const DnsQuery &query, quint8 rcode, quint16 flags)
{
    m_rcode = rcode;
    m_response = createResponse(query, rcode, flags);
}

void DnsAnswer::setError(const DnsQuery &query, const QString &error)
{
    setRcode(query, ARES_RCODE_SERVFAIL);

    m_error = error;
}

void DnsAnswer::readAnswers(const ares_dns_record *record)
{
    const size_t count = ares_dns_record_rr_cnt(record, ARES_SECTION_ANSWER);

    for (size_t i = 0; i < count; ++i) {
        const ares_dns_rr_t *rr = ares_dns_record_rr_get_const(record, ARES_SECTION_ANSWER, i);

        readTargetName(rr);

        if (readAddress(rr)) {
            m_addresses.addTtl(ares_dns_rr_get_ttl(rr));
        }
    }
}

void DnsAnswer::readTargetName(const ares_dns_rr *rr)
{
    const char *name = targetName(rr);

    // The root name "." (of the service mode's owner name or the alias mode's no service) is
    // parsed as empty
    if (name == nullptr || *name == '\0')
        return;

    m_targetNames.append(QString::fromLatin1(name));
}

bool DnsAnswer::readAddress(const ares_dns_rr *rr)
{
    switch (ares_dns_rr_get_type(rr)) {
    case ARES_REC_TYPE_A: {
        const in_addr *addr = ares_dns_rr_get_addr(rr, ARES_RR_A_ADDR);

        m_addresses.addIp4(qFromBigEndian<quint32>(&addr->s_addr));
    } break;
    case ARES_REC_TYPE_AAAA: {
        const ares_in6_addr *addr = ares_dns_rr_get_addr6(rr, ARES_RR_AAAA_ADDR);

        m_addresses.addIp6(addr->_S6_un._S6_u8);
    } break;
    case ARES_REC_TYPE_SVCB:
        return readHints(m_addresses, rr, ARES_RR_SVCB_PARAMS);
    case ARES_REC_TYPE_HTTPS:
        return readHints(m_addresses, rr, ARES_RR_HTTPS_PARAMS);
    default:
        return false;
    }

    return true;
}
