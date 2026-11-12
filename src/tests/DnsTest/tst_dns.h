#pragma once

#include <QEventLoop>
#include <QNetworkDatagram>
#include <QUdpSocket>

#include <memory>

#include <ares.h>

#include <googletest.h>

#include <dns/dnsanswer.h>
#include <dns/dnsclientanswer.h>
#include <dns/dnsquery.h>
#include <dns/dnsresolver.h>
#include <dns/dnsutil.h>
#include <util/net/netformatutil.h>

class DnsTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void DnsTest::SetUp() { }

void DnsTest::TearDown() { }

namespace {

DnsRecordPtr createRecord(quint16 id, quint16 flags, const char *name)
{
    ares_dns_record_t *record = nullptr;
    ares_dns_record_create(&record, id, flags, ARES_OPCODE_QUERY, ARES_RCODE_NOERROR);
    ares_dns_record_query_add(record, name, ARES_REC_TYPE_A, ARES_CLASS_IN);

    return DnsRecordPtr(record, &ares_dns_record_destroy);
}

ares_dns_rr_t *addOpt(ares_dns_record_t *record, quint16 udpSize)
{
    ares_dns_rr_t *rr = nullptr;
    ares_dns_record_rr_add(
            &rr, record, ARES_SECTION_ADDITIONAL, "", ARES_REC_TYPE_OPT, ARES_CLASS_IN, 0);
    ares_dns_rr_set_u16(rr, ARES_RR_OPT_UDP_SIZE, udpSize);
    ares_dns_rr_set_u8(rr, ARES_RR_OPT_VERSION, 0);
    ares_dns_rr_set_u16(rr, ARES_RR_OPT_FLAGS, 0);

    return rr;
}

QByteArray writeQuery(quint16 id, const char *name, bool withOpt)
{
    const DnsRecordPtr record = createRecord(id, ARES_FLAG_RD, name);

    if (withOpt) {
        addOpt(record.get(), 4096);
    }

    return DnsUtil::writeRecord(record.get());
}

void addAddressRr(ares_dns_record_t *record, const char *name, const char *ip, quint32 ttl)
{
    const bool isIp6 = strchr(ip, ':') != nullptr;
    ares_dns_rr_t *rr = nullptr;

    ares_dns_record_rr_add(&rr, record, ARES_SECTION_ANSWER, name,
            isIp6 ? ARES_REC_TYPE_AAAA : ARES_REC_TYPE_A, ARES_CLASS_IN, ttl);

    if (isIp6) {
        ares_in6_addr addr;
        ares_inet_pton(AF_INET6, ip, &addr);
        ares_dns_rr_set_addr6(rr, ARES_RR_AAAA_ADDR, &addr);
    } else {
        in_addr addr;
        ares_inet_pton(AF_INET, ip, &addr);
        ares_dns_rr_set_addr(rr, ARES_RR_A_ADDR, &addr);
    }
}

// The upstream's answer with a CNAME and an EDNS cookie
DnsRecordPtr createUpstreamAnswer()
{
    DnsRecordPtr record =
            createRecord(777, ARES_FLAG_QR | ARES_FLAG_RD | ARES_FLAG_RA, "www.example.com");

    ares_dns_rr_t *rr = nullptr;
    ares_dns_record_rr_add(&rr, record.get(), ARES_SECTION_ANSWER, "www.example.com",
            ARES_REC_TYPE_CNAME, ARES_CLASS_IN, 100);
    ares_dns_rr_set_str(rr, ARES_RR_CNAME_CNAME, "cdn.example.net");

    addAddressRr(record.get(), "cdn.example.net", "1.2.3.4", 60);
    addAddressRr(record.get(), "cdn.example.net", "5.6.7.8", 300);
    addAddressRr(record.get(), "cdn.example.net", "2001:db8::1", 30);

    ares_dns_rr_t *optRr = addOpt(record.get(), 1232);
    const unsigned char cookie[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    ares_dns_rr_set_opt(optRr, ARES_RR_OPT_OPTIONS, ARES_OPT_PARAM_COOKIE, cookie, sizeof(cookie));

    return record;
}

}

TEST_F(DnsTest, queryParse)
{
    DnsQuery query;

    ASSERT_TRUE(query.parse(writeQuery(0x1234, "Example.COM", /*withOpt=*/false)));
    ASSERT_EQ(query.id(), 0x1234);
    ASSERT_EQ(query.name(), "Example.COM");
    ASSERT_EQ(query.errorRcode(), ARES_RCODE_NOERROR);
    ASSERT_FALSE(query.hasOpt());
    ASSERT_EQ(query.udpSizeMax(), 512);

    ASSERT_TRUE(query.parse(writeQuery(1, "example.com", /*withOpt=*/true)));
    ASSERT_TRUE(query.hasOpt());
    ASSERT_EQ(query.udpSizeMax(), 4096);

    // Not a query
    ASSERT_FALSE(query.parse("bad"));
    ASSERT_FALSE(query.parse(DnsUtil::writeRecord(createUpstreamAnswer().get())));

    // Two questions
    {
        const DnsRecordPtr record = createRecord(2, 0, "a.com");
        ares_dns_record_query_add(record.get(), "b.com", ARES_REC_TYPE_A, ARES_CLASS_IN);

        ASSERT_FALSE(query.parse(DnsUtil::writeRecord(record.get())));
    }

    // Not a standard query
    {
        ares_dns_record_t *record = nullptr;
        ares_dns_record_create(&record, 3, 0, ARES_OPCODE_STATUS, ARES_RCODE_NOERROR);
        ares_dns_record_query_add(record, "a.com", ARES_REC_TYPE_A, ARES_CLASS_IN);

        const DnsRecordPtr recordPtr(record, &ares_dns_record_destroy);

        ASSERT_TRUE(query.parse(DnsUtil::writeRecord(record)));
        ASSERT_EQ(query.errorRcode(), ARES_RCODE_NOTIMP);
    }
}

TEST_F(DnsTest, rcodeAnswer)
{
    DnsQuery query;
    ASSERT_TRUE(query.parse(writeQuery(0x4321, "blocked.com", /*withOpt=*/false)));

    DnsAnswer answer;
    answer.setRcode(query, ARES_RCODE_NXDOMAIN);

    ASSERT_TRUE(answer.addresses().isEmpty());

    const DnsRecordPtr record = DnsUtil::parseRecord(answer.response());
    ASSERT_NE(record, nullptr);

    ASSERT_EQ(ares_dns_record_get_id(record.get()), 0x4321);
    ASSERT_EQ(ares_dns_record_get_flags(record.get()), ARES_FLAG_QR | ARES_FLAG_RD | ARES_FLAG_RA);
    ASSERT_EQ(ares_dns_record_get_rcode(record.get()), ARES_RCODE_NXDOMAIN);

    const char *name = nullptr;
    ASSERT_EQ(ares_dns_record_query_get(record.get(), 0, &name, nullptr, nullptr), ARES_SUCCESS);
    ASSERT_STREQ(name, "blocked.com");

    ASSERT_EQ(ares_dns_record_rr_cnt(record.get(), ARES_SECTION_AUTHORITY), 1);
    ASSERT_EQ(ares_dns_rr_get_type(
                      ares_dns_record_rr_get_const(record.get(), ARES_SECTION_AUTHORITY, 0)),
            ARES_REC_TYPE_SOA);
    ASSERT_EQ(DnsUtil::optIndex(record.get()), -1);

    // With EDNS
    ASSERT_TRUE(query.parse(writeQuery(1, "blocked.com", /*withOpt=*/true)));
    answer.setRcode(query, ARES_RCODE_SERVFAIL);

    const DnsRecordPtr optRecord = DnsUtil::parseRecord(answer.response());
    ASSERT_EQ(ares_dns_record_get_rcode(optRecord.get()), ARES_RCODE_SERVFAIL);
    ASSERT_EQ(ares_dns_record_rr_cnt(optRecord.get(), ARES_SECTION_AUTHORITY), 0);
    ASSERT_EQ(DnsUtil::optIndex(optRecord.get()), 0);
}

TEST_F(DnsTest, recordAnswer)
{
    const DnsRecordPtr upstreamRecord = createUpstreamAnswer();

    DnsQuery query;
    ASSERT_TRUE(query.parse(writeQuery(0x5555, "www.example.com", /*withOpt=*/false)));

    DnsAnswer answer;
    answer.setRecord(query, upstreamRecord.get(), query.udpSizeMax());

    const DnsAddresses &addresses = answer.addresses();
    ASSERT_FALSE(addresses.isEmpty());
    ASSERT_EQ(addresses.ttl(), 300);
    ASSERT_EQ(addresses.ip4Array(),
            QVector<quint32>(
                    { NetFormatUtil::textToIp4("1.2.3.4"), NetFormatUtil::textToIp4("5.6.7.8") }));
    ASSERT_EQ(addresses.ip6Array().size(), 1);
    ASSERT_EQ(NetFormatUtil::ip6ToText(addresses.ip6Array().at(0)), "2001:db8::1");

    // The client's id, without EDNS
    {
        const DnsRecordPtr record = DnsUtil::parseRecord(answer.response());
        ASSERT_NE(record, nullptr);

        ASSERT_EQ(ares_dns_record_get_id(record.get()), 0x5555);
        ASSERT_EQ(ares_dns_record_rr_cnt(record.get(), ARES_SECTION_ANSWER), 4);
        ASSERT_EQ(DnsUtil::optIndex(record.get()), -1);
    }

    // With EDNS: without the cookie
    {
        ASSERT_TRUE(query.parse(writeQuery(0x6666, "www.example.com", /*withOpt=*/true)));
        answer.setRecord(query, upstreamRecord.get(), query.udpSizeMax());

        const DnsRecordPtr record = DnsUtil::parseRecord(answer.response());
        const int optIndex = DnsUtil::optIndex(record.get());
        ASSERT_GE(optIndex, 0);

        const ares_dns_rr_t *rr =
                ares_dns_record_rr_get_const(record.get(), ARES_SECTION_ADDITIONAL, optIndex);
        ASSERT_EQ(ares_dns_rr_get_u16(rr, ARES_RR_OPT_UDP_SIZE), 1232);
        ASSERT_FALSE(ares_dns_rr_get_opt_byid(
                rr, ARES_RR_OPT_OPTIONS, ARES_OPT_PARAM_COOKIE, nullptr, nullptr));
    }

    // Truncated
    {
        DnsAnswer truncatedAnswer;
        truncatedAnswer.setRecord(query, upstreamRecord.get(), /*sizeMax=*/32);

        const DnsRecordPtr record = DnsUtil::parseRecord(truncatedAnswer.response());
        ASSERT_NE(record, nullptr);

        ASSERT_EQ(ares_dns_record_get_id(record.get()), 0x6666);
        ASSERT_NE(ares_dns_record_get_flags(record.get()) & ARES_FLAG_TC, 0);
        ASSERT_EQ(ares_dns_record_rr_cnt(record.get(), ARES_SECTION_ANSWER), 0);
    }
}

TEST_F(DnsTest, hintsAnswer)
{
    const auto createHttpsRecord = [](quint16 flags) {
        ares_dns_record_t *record = nullptr;
        ares_dns_record_create(&record, 0x8888, flags, ARES_OPCODE_QUERY, ARES_RCODE_NOERROR);
        ares_dns_record_query_add(record, "www.example.com", ARES_REC_TYPE_HTTPS, ARES_CLASS_IN);

        return DnsRecordPtr(record, &ares_dns_record_destroy);
    };

    const auto addSvcbRr = [](ares_dns_record_t *record, ares_dns_rec_type_t type,
                                   const char *target, quint32 ttl) {
        ares_dns_rr_t *rr = nullptr;
        ares_dns_record_rr_add(
                &rr, record, ARES_SECTION_ANSWER, "www.example.com", type, ARES_CLASS_IN, ttl);

        const bool isHttps = (type == ARES_REC_TYPE_HTTPS);
        ares_dns_rr_set_u16(rr, isHttps ? ARES_RR_HTTPS_PRIORITY : ARES_RR_SVCB_PRIORITY,
                *target == '.' ? 1 : 0);
        ares_dns_rr_set_str(rr, isHttps ? ARES_RR_HTTPS_TARGET : ARES_RR_SVCB_TARGET, target);

        return rr;
    };

    const DnsRecordPtr record = createHttpsRecord(ARES_FLAG_QR | ARES_FLAG_RD | ARES_FLAG_RA);

    // The HTTPS record with an ALPN and both hints
    {
        ares_dns_rr_t *rr = addSvcbRr(record.get(), ARES_REC_TYPE_HTTPS, ".", 120);

        const unsigned char alpn[] = { 2, 'h', '2' };
        ares_dns_rr_set_opt(rr, ARES_RR_HTTPS_PARAMS, ARES_SVCB_PARAM_ALPN, alpn, sizeof(alpn));

        const unsigned char ip4Hint[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        ares_dns_rr_set_opt(
                rr, ARES_RR_HTTPS_PARAMS, ARES_SVCB_PARAM_IPV4HINT, ip4Hint, sizeof(ip4Hint));

        const unsigned char ip6Hint[] = { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1 };
        ares_dns_rr_set_opt(
                rr, ARES_RR_HTTPS_PARAMS, ARES_SVCB_PARAM_IPV6HINT, ip6Hint, sizeof(ip6Hint));
    }

    // The HTTPS record of the alias mode, without hints: its TTL isn't of the addresses
    addSvcbRr(record.get(), ARES_REC_TYPE_HTTPS, "cdn.example.net", 600);

    // The SVCB record with an IPv4 hint
    {
        ares_dns_rr_t *rr = addSvcbRr(record.get(), ARES_REC_TYPE_SVCB, ".", 60);

        const unsigned char ip4Hint[] = { 9, 9, 9, 9 };
        ares_dns_rr_set_opt(
                rr, ARES_RR_SVCB_PARAMS, ARES_SVCB_PARAM_IPV4HINT, ip4Hint, sizeof(ip4Hint));
    }

    DnsQuery query;
    ASSERT_TRUE(query.parse(DnsUtil::writeRecord(createHttpsRecord(ARES_FLAG_RD).get())));

    // The parameters as received
    const DnsRecordPtr parsedRecord = DnsUtil::parseRecord(DnsUtil::writeRecord(record.get()));
    ASSERT_NE(parsedRecord, nullptr);

    DnsAnswer answer;
    answer.setRecord(query, parsedRecord.get(), query.udpSizeMax());

    const DnsAddresses &addresses = answer.addresses();
    ASSERT_EQ(addresses.ttl(), 120);
    ASSERT_EQ(addresses.ip4Array(),
            QVector<quint32>({ NetFormatUtil::textToIp4("1.2.3.4"),
                    NetFormatUtil::textToIp4("5.6.7.8"), NetFormatUtil::textToIp4("9.9.9.9") }));
    ASSERT_EQ(addresses.ip6Array().size(), 1);
    ASSERT_EQ(NetFormatUtil::ip6ToText(addresses.ip6Array().at(0)), "2001:db8::1");
}

TEST_F(DnsTest, resolverOwnQuery)
{
    // The upstream server
    QUdpSocket server;
    ASSERT_TRUE(server.bind(QHostAddress::LocalHost, 0));

    DnsResolver resolver;
    ASSERT_TRUE(resolver.open(QString("127.0.0.1:%1").arg(server.localPort())));

    DnsRequest request;
    ASSERT_TRUE(request.query.parse(writeQuery(0x1111, "loop.example", /*withOpt=*/false)));

    QEventLoop loop;
    DnsAnswer resolvedAnswer;
    QObject::connect(&resolver, &DnsResolver::resolved, &loop,
            [&](const DnsRequest & /*request*/, const DnsAnswer &answer) {
                resolvedAnswer = answer;
                loop.quit();
            });

    resolver.resolve(request);

    ASSERT_TRUE(server.waitForReadyRead());

    const QNetworkDatagram datagram = server.receiveDatagram();

    // The resolver's query is its own one while it waits for the answer
    DnsQuery ownQuery;
    ASSERT_TRUE(ownQuery.parse(datagram.data()));
    ASSERT_EQ(ownQuery.name(), "loop.example");
    ASSERT_TRUE(resolver.isOwnQuery(ownQuery));

    // The client's query of another type with the same id
    const DnsRecordPtr otherRecord = DnsUtil::duplicateRecord(ownQuery.record(), ownQuery.id());
    ares_dns_record_query_set_type(otherRecord.get(), 0, ARES_REC_TYPE_AAAA);

    DnsQuery otherQuery;
    ASSERT_TRUE(otherQuery.parse(DnsUtil::writeRecord(otherRecord.get())));
    ASSERT_FALSE(resolver.isOwnQuery(otherQuery));

    DnsAnswer serverAnswer;
    serverAnswer.setRcode(ownQuery, ARES_RCODE_NXDOMAIN);
    server.writeDatagram(serverAnswer.response(), datagram.senderAddress(), datagram.senderPort());

    loop.exec();

    ASSERT_FALSE(resolver.isOwnQuery(ownQuery));

    // The client's id in the answer
    const DnsRecordPtr record = DnsUtil::parseRecord(resolvedAnswer.response());
    ASSERT_NE(record, nullptr);
    ASSERT_EQ(ares_dns_record_get_id(record.get()), 0x1111);
    ASSERT_EQ(ares_dns_record_get_rcode(record.get()), ARES_RCODE_NXDOMAIN);
}

TEST_F(DnsTest, clientAnswerQueryResults)
{
    DnsClientAnswer answer;
    answer.readQueryResults(
            u"type: 5 example.cdn.net; ::ffff:1.2.3.4;2001:db8::1;type: 16 text;bad;;5.6.7.8;");

    const DnsAddresses &addresses = answer.addresses();
    ASSERT_EQ(addresses.ip4Array(),
            QVector<quint32>(
                    { NetFormatUtil::textToIp4("1.2.3.4"), NetFormatUtil::textToIp4("5.6.7.8") }));
    ASSERT_EQ(addresses.ip6Array().size(), 1);
    ASSERT_EQ(NetFormatUtil::ip6ToText(addresses.ip6Array().at(0)), "2001:db8::1");

    // Without addresses
    DnsClientAnswer emptyAnswer;
    emptyAnswer.readQueryResults(u"type: 5 alias.example.com;");
    ASSERT_TRUE(emptyAnswer.addresses().isEmpty());
}
