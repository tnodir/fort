#pragma once

#include <googletest.h>

#include <common/fortconf.h>

#include <util/conf/confbuffer.h>
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

TEST_F(FilterLineTest, singleLine)
{
    FilterLine line;
    ASSERT_TRUE(line.parse());
    ASSERT_TRUE(line.isSingleLine());

    FilterLine line2("Port(80)");
    ASSERT_TRUE(line2.parse());
    ASSERT_TRUE(line2.isSingleLine());

    FilterLine line3("Host(\na.com\n):Port(80, 443)");
    ASSERT_TRUE(line3.parse());
    ASSERT_TRUE(line3.isSingleLine());

    // Lines of filters
    FilterLine line4("Host(\na.com\n)\nPort(80)");
    ASSERT_TRUE(line4.parse());
    ASSERT_FALSE(line4.isSingleLine());

    FilterLine line5("Host(\na.com\n)\n;\nPort(80)");
    ASSERT_TRUE(line5.parse());
    ASSERT_FALSE(line5.isSingleLine());
}

TEST_F(FilterLineTest, valuesLines)
{
    FilterLineText text;
    text.addFilter(FORT_RULE_FILTER_TYPE_HOST, "# CDN (old)\na.com\n\nb.com # main");
    text.addFilter(FORT_RULE_FILTER_TYPE_ADDRESS, "1.1.1.1 # DNS");
    text.addFilter(FORT_RULE_FILTER_TYPE_PORT, "80, 443");
    text.addFilter(FORT_RULE_FILTER_TYPE_ACTION, "Allow");
    ASSERT_EQ(text.text(),
            "Host(\n# CDN (old)\na.com\n\nb.com # main\n):IP(\n1.1.1.1 # DNS\n)"
            ":Port(80, 443):Act(Allow)");

    ASSERT_TRUE(ConfBuffer().validateRuleText(text.text()));

    FilterLine line(text);
    ASSERT_TRUE(line.parse());
    ASSERT_TRUE(line.isSingleLine());

    checkFilter(line, FORT_RULE_FILTER_TYPE_HOST, { "a.com", "b.com" });
    checkFilter(line, FORT_RULE_FILTER_TYPE_ADDRESS, { "1.1.1.1" });
    checkFilter(line, FORT_RULE_FILTER_TYPE_ACTION, { "Allow" });

    ASSERT_EQ(FilterLine::valuesLines(line.filter(FORT_RULE_FILTER_TYPE_HOST)),
            "# CDN (old)\na.com\n\nb.com # main");
    ASSERT_EQ(FilterLine::valuesLines(line.filter(FORT_RULE_FILTER_TYPE_ADDRESS)), "1.1.1.1 # DNS");
    ASSERT_EQ(FilterLine::valuesLines(line.filter(FORT_RULE_FILTER_TYPE_PORT)), "80\n443");
    ASSERT_EQ(FilterLine::valuesLines(nullptr), QString());
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

TEST_F(FilterLineTest, connAddressesText)
{
    Conn conn;
    conn.ipProto = 6; // TCP
    conn.localPort = 50000;
    conn.remotePort = 443;
    conn.localIp.v4 = NetFormatUtil::textToIp4("192.168.1.2");
    conn.remoteIp.v4 = NetFormatUtil::textToIp4("1.1.1.1");

    FilterLineText line;
    line.addConnAddresses(conn);
    ASSERT_EQ(line.text(),
            "Dir(Out):Proto(TCP):IP(1.1.1.1):Port(443):Local_IP(192.168.1.2)"
            ":Local_Port(50000)");
    ASSERT_TRUE(FilterLine(line).parse());

    conn.isIPv6 = true;
    conn.inbound = true;
    conn.ipProto = 58; // ICMPv6
    conn.localPort = 0;
    conn.remotePort = 0;
    conn.localIp.v6 = NetFormatUtil::textToIp6("fe80::1");
    conn.remoteIp.v6 = NetFormatUtil::textToIp6("fe80::2");

    FilterLineText line2;
    line2.addConnAddresses(conn);
    ASSERT_EQ(line2.text(), "Dir(In):Proto(ICMPv6):IP([fe80::2]):Local_IP([fe80::1])");
}

TEST_F(FilterLineTest, equalText)
{
    FilterLineText line;
    line.addFilter(FORT_RULE_FILTER_TYPE_PROTOCOL, "TCP");
    line.addFilter(
            FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS, QString(), /*isNot=*/true, /*isEqual=*/true);
    line.addFilter(FORT_RULE_FILTER_TYPE_LOCAL_PORT, QString(), /*isNot=*/false, /*isEqual=*/true);
    line.addFilter(FORT_RULE_FILTER_TYPE_ACTION, "Allow");
    ASSERT_EQ(line.text(), "Proto(TCP):!=Local_IP:=Local_Port:Act(Allow)");

    ASSERT_TRUE(ConfBuffer().validateRuleText(line.text()));

    FilterLine filterLine(line);
    ASSERT_TRUE(filterLine.parse());

    checkFilter(filterLine, FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS, {}, /*isNot=*/true);
    ASSERT_TRUE(filterLine.filter(FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS)->equalValues);

    checkFilter(filterLine, FORT_RULE_FILTER_TYPE_LOCAL_PORT, {});
    ASSERT_TRUE(filterLine.filter(FORT_RULE_FILTER_TYPE_LOCAL_PORT)->equalValues);

    checkFilter(filterLine, FORT_RULE_FILTER_TYPE_ACTION, { "Allow" });
}

TEST_F(FilterLineTest, hostText)
{
    FilterLineText line;
    line.addFilter(FORT_RULE_FILTER_TYPE_HOST, "example.com, *.cdn.net", /*isNot=*/true);
    line.addFilter(FORT_RULE_FILTER_TYPE_PORT, "443");
    line.addFilter(FORT_RULE_FILTER_TYPE_ACTION, "Block");
    ASSERT_EQ(line.text(), "!Host(example.com, *.cdn.net):Port(443):Act(Block)");

    ASSERT_TRUE(ConfBuffer().validateRuleText(line.text()));

    FilterLine filterLine(line);
    ASSERT_TRUE(filterLine.parse());

    checkFilter(
            filterLine, FORT_RULE_FILTER_TYPE_HOST, { "example.com", "*.cdn.net" }, /*isNot=*/true);
    checkFilter(filterLine, FORT_RULE_FILTER_TYPE_PORT, { "443" });
}
