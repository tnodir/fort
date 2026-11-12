#include "dnsutil.h"

#include <ares.h>

int DnsUtil::optIndex(const ares_dns_record *record)
{
    const size_t count = ares_dns_record_rr_cnt(record, ARES_SECTION_ADDITIONAL);

    for (size_t i = 0; i < count; ++i) {
        const ares_dns_rr_t *rr = ares_dns_record_rr_get_const(record, ARES_SECTION_ADDITIONAL, i);

        if (ares_dns_rr_get_type(rr) == ARES_REC_TYPE_OPT)
            return int(i);
    }

    return -1;
}

DnsRecordPtr DnsUtil::parseRecord(const QByteArray &data)
{
    ares_dns_record_t *record = nullptr;

    ares_dns_parse((const unsigned char *) data.constData(), data.size(), /*flags=*/0, &record);

    return DnsRecordPtr(record, &ares_dns_record_destroy);
}

DnsRecordPtr DnsUtil::duplicateRecord(const ares_dns_record *record, quint16 id)
{
    DnsRecordPtr copy(ares_dns_record_duplicate(record), &ares_dns_record_destroy);

    if (copy) {
        ares_dns_record_set_id(copy.get(), id);
    }

    return copy;
}

QByteArray DnsUtil::writeRecord(const ares_dns_record *record)
{
    unsigned char *buf = nullptr;
    size_t len = 0;

    if (ares_dns_write(record, &buf, &len) != ARES_SUCCESS)
        return {};

    const QByteArray data((const char *) buf, qsizetype(len));

    ares_free_string(buf);

    return data;
}
