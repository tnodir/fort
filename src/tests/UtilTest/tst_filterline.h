#pragma once

#include <googletest.h>

#include <common/fortconf.h>

#include <util/conf/conn.h>
#include <util/conf/filterline.h>
#include <util/conf/filterlinetext.h>
#include <util/conf/ruletextparser.h>
#include <util/net/netformatutil.h>

class FilterLineTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();

protected:
    void checkFilter(
            const FilterLine &line, qint8 type, const QStringList &values, bool isNot = false);
};

void FilterLineTest::SetUp() { }

void FilterLineTest::TearDown() { }

void FilterLineTest::checkFilter(
        const FilterLine &line, qint8 type, const QStringList &values, bool isNot)
{
    const RuleFilter *filter = line.filter(type);
    ASSERT_NE(filter, nullptr);
    ASSERT_EQ(filter->isNot, isNot);

    QStringList filterValues;
    for (const QStringView value : filter->values) {
        filterValues << value.toString();
    }

    ASSERT_EQ(filterValues, values);
}

TEST_F(FilterLineTest, parse)
{
    FilterLine line;
    ASSERT_TRUE(line.parse());
    ASSERT_EQ(line.filter(FORT_RULE_FILTER_TYPE_ADDRESS), nullptr);

    FilterLine line2("act(drop):dir(in):port(53):ip(1.1.1.1)");
    ASSERT_TRUE(line2.parse());

    checkFilter(line2, FORT_RULE_FILTER_TYPE_ADDRESS, { "1.1.1.1" });
    checkFilter(line2, FORT_RULE_FILTER_TYPE_PORT, { "53" });
    checkFilter(line2, FORT_RULE_FILTER_TYPE_DIRECTION, { "in" });
    checkFilter(line2, FORT_RULE_FILTER_TYPE_ACTION, { "drop" });
    ASSERT_EQ(line2.filter(FORT_RULE_FILTER_TYPE_PROTOCOL), nullptr);

    FilterLine line3("!(1.1.1.1, [::1]):!80");
    ASSERT_TRUE(line3.parse());

    checkFilter(line3, FORT_RULE_FILTER_TYPE_ADDRESS, { "1.1.1.1", "[::1]" }, /*isNot=*/true);
    checkFilter(line3, FORT_RULE_FILTER_TYPE_PORT, { "80" }, /*isNot=*/true);

    FilterLine line4("1.1.1.1:(");
    ASSERT_FALSE(line4.parse());
    ASSERT_EQ(line4.filter(FORT_RULE_FILTER_TYPE_ADDRESS), nullptr);
}

TEST_F(FilterLineTest, connText)
{
    Conn conn;
    conn.ipProto = 6; // TCP
    conn.localPort = 50000;
    conn.remotePort = 443;
    conn.localIp.v4 = NetFormatUtil::textToIp4("192.168.1.2");
    conn.remoteIp.v4 = NetFormatUtil::textToIp4("1.1.1.1");

    FilterLineText line(conn);
    ASSERT_EQ(line.text(),
            "Dir(Out):Proto(TCP):IP(1.1.1.1):Port(443):Local_IP(192.168.1.2)"
            ":Local_Port(50000):Act(Block)");

    conn.blocked = true;

    FilterLineText line2(conn);
    ASSERT_EQ(line2.text(),
            "Dir(Out):Proto(TCP):IP(1.1.1.1):Port(443):Local_IP(192.168.1.2)"
            ":Local_Port(50000):Act(Allow)");
    ASSERT_TRUE(FilterLine(line2).parse());
}
