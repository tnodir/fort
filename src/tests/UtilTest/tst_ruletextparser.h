#pragma once

#include <QDebug>

#include <googletest.h>

#include <common/fortconf.h>

#include <util/conf/ruletextparser.h>

class RuleTextParserTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();

protected:
    void checkStringList(const StringViewList &l1, const QStringList &l2);
    void checkRuleFilters(const QString &text1, const QString &text2);
};

void RuleTextParserTest::SetUp() { }

void RuleTextParserTest::TearDown() { }

void RuleTextParserTest::checkStringList(const StringViewList &l1, const QStringList &l2)
{
    ASSERT_EQ(l1.size(), l2.size());

    for (int i = 0; i < l1.size(); ++i) {
        const QStringView &s1 = l1[i];
        const QString &s2 = l2[i];

        if (s1 != QStringView(s2)) {
            ASSERT_EQ(s1.toString(), s2);
        }
    }
}

void RuleTextParserTest::checkRuleFilters(const QString &text1, const QString &text2)
{
    RuleTextParser p1(text1);
    RuleTextParser p2(text2);

    ASSERT_TRUE(p1.parse());
    ASSERT_TRUE(p2.parse());

    const auto &ruleFilters1 = p1.ruleFilters();
    const auto &ruleFilters2 = p2.ruleFilters();

    ASSERT_EQ(ruleFilters1.size(), ruleFilters2.size());

    for (int i = 0; i < ruleFilters1.size(); ++i) {
        ASSERT_EQ(ruleFilters1[i].type, ruleFilters2[i].type);
        ASSERT_EQ(ruleFilters1[i].values, ruleFilters2[i].values);
    }
}

TEST_F(RuleTextParserTest, emptyList)
{
    RuleTextParser p("{}");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 0);
}

TEST_F(RuleTextParserTest, emptyListDepth)
{
    RuleTextParser p("{{{{{{{}}}}}}}");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 0);
}

TEST_F(RuleTextParserTest, maxListDepth)
{
    RuleTextParser p("{{{{{{{{{{");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorListMaxDepth);
}

TEST_F(RuleTextParserTest, emptyComment)
{
    RuleTextParser p("#");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 0);
}

TEST_F(RuleTextParserTest, setText)
{
    RuleTextParser p("{1.1.1.1:53");

    ASSERT_FALSE(p.parse());

    p.setText("1.1.1.1:53");

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorNone);
    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);
}

TEST_F(RuleTextParserTest, lineIpPort)
{
    RuleTextParser p("1.1.1.1:53");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1.1.1.1" });
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT);
        checkStringList(rf.values, { "53" });
    }
}

TEST_F(RuleTextParserTest, lineIpValues)
{
    RuleTextParser p("(1.1.1.1/8, [2::]/16)");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 1);

    // Check IP Values
    {
        const RuleFilter &rf = p.ruleFilters()[0];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1.1.1.1/8", "[2::]/16" });
    }
}

TEST_F(RuleTextParserTest, lineIpPortList)
{
    RuleTextParser p("1.1.1.1:53\n"
                     "2.2.2.2:64\n"
                     "3.3.3.3:75\n");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 10);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[5];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "2.2.2.2" });
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[6];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT);
        checkStringList(rf.values, { "64" });
    }

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[8];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "3.3.3.3" });
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[9];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT);
        checkStringList(rf.values, { "75" });
    }
}

TEST_F(RuleTextParserTest, lineCrLf)
{
    checkRuleFilters("# comment\r\n"
                     "1.1.1.1:53\r\n"
                     "dir(in):tcp(80)\r\n",
            "# comment\n"
            "1.1.1.1:53\n"
            "dir(in):tcp(80)\n");
}

TEST_F(RuleTextParserTest, lineBom)
{
    checkRuleFilters(QChar(QChar::ByteOrderMark) + QString("1.1.1.1:53\n"), "1.1.1.1:53\n");
}

TEST_F(RuleTextParserTest, filterDirUdp)
{
    RuleTextParser p("dir(out):udp(53)");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);

    // Check Direction
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_DIRECTION);
        checkStringList(rf.values, { "out" });
    }

    // Check UDP Port
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT_UDP);
        checkStringList(rf.values, { "53" });
    }
}

TEST_F(RuleTextParserTest, filterZones)
{
    RuleTextParser p("zones()");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 1);

    // Check Zones
    {
        const RuleFilter &rf = p.ruleFilters()[0];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ZONES);
        ASSERT_TRUE(rf.values.isEmpty());
    }
}

TEST_F(RuleTextParserTest, filterArea)
{
    RuleTextParser p("area(localhost,lan,inet)");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 1);

    // Check Area
    {
        const RuleFilter &rf = p.ruleFilters()[0];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_AREA);
        checkStringList(rf.values, { "localhost", "lan", "inet" });
    }
}

TEST_F(RuleTextParserTest, lineSectionList)
{
    RuleTextParser p("ip(\n#1\n1.1.1.1/8\n#2\n2.2.2.2/16\n):{\ntcp(80)\n}");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1.1.1.1/8", "2.2.2.2/16" });
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT_TCP);
        checkStringList(rf.values, { "80" });
    }
}

TEST_F(RuleTextParserTest, lineIp6Range)
{
    RuleTextParser p("[2:]/3-[4:]/5:67");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "[2:]/3-[4:]/5" });
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT);
        checkStringList(rf.values, { "67" });
    }
}

TEST_F(RuleTextParserTest, lineEmptySections)
{
    RuleTextParser p("1:::2::\n");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);
}

TEST_F(RuleTextParserTest, badStartOfLine)
{
    RuleTextParser p(":1\n");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorUnexpectedStartOfLine);
}

TEST_F(RuleTextParserTest, badNoFilterName)
{
    RuleTextParser p("1:2:3");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorNoFilterName);
}

TEST_F(RuleTextParserTest, badFilterName)
{
    RuleTextParser p("test(1)");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorBadFilterName);
}

TEST_F(RuleTextParserTest, badEndOfValueBegin)
{
    RuleTextParser p("[1[");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorUnexpectedEndOfValue);
}

TEST_F(RuleTextParserTest, badEndOfValueEnd)
{
    RuleTextParser p("[1]]");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorUnexpectedStartOfLine);
}

TEST_F(RuleTextParserTest, badEndOfList)
{
    RuleTextParser p("{1");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorUnexpectedEndOfList);
}

TEST_F(RuleTextParserTest, badEndOfValuesList)
{
    RuleTextParser p("(1");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorUnexpectedEndOfValuesList);
}

TEST_F(RuleTextParserTest, badSymboOfListEnd)
{
    RuleTextParser p("1}");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorUnexpectedSymboOfListEnd);
}

TEST_F(RuleTextParserTest, badExtraFilterName)
{
    RuleTextParser p("ip dir(1)");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorExtraFilterName);
}

TEST_F(RuleTextParserTest, badBadSymbol)
{
    RuleTextParser p("1\b");

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorBadSymbol);
}

TEST_F(RuleTextParserTest, badNonLatin1Symbol)
{
    RuleTextParser p(QString("1.1.1.1") + QChar(0x2192)); // arrow

    ASSERT_FALSE(p.parse());

    ASSERT_EQ(p.errorCode(), RuleTextParser::ErrorBadSymbol);
}

TEST_F(RuleTextParserTest, filterNot)
{
    RuleTextParser p("!!!1");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 1);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[0];
        ASSERT_TRUE(rf.isNot);
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1" });
    }
}

TEST_F(RuleTextParserTest, listNot)
{
    // The list's negation
    {
        RuleTextParser p("!{1\n2}");

        ASSERT_TRUE(p.parse());

        ASSERT_EQ(p.ruleFilters().size(), 5);

        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_TRUE(rf.isNot);
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_LIST_OR);
    }

    // The collapsed list's negation
    {
        RuleTextParser p("!{1}");

        ASSERT_TRUE(p.parse());

        ASSERT_EQ(p.ruleFilters().size(), 1);

        const RuleFilter &rf = p.ruleFilters()[0];
        ASSERT_TRUE(rf.isNot);
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1" });
    }

    // The collapsed list's double negation
    {
        RuleTextParser p("!{!1}");

        ASSERT_TRUE(p.parse());

        ASSERT_EQ(p.ruleFilters().size(), 1);

        const RuleFilter &rf = p.ruleFilters()[0];
        ASSERT_FALSE(rf.isNot);
    }
}

TEST_F(RuleTextParserTest, lineEndNot)
{
    RuleTextParser p("1!2");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1" });
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_TRUE(rf.isNot);
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT);
        checkStringList(rf.values, { "2" });
    }
}

TEST_F(RuleTextParserTest, lineIpPortNameList)
{
    RuleTextParser p("1.1.1.1:udp(53)\n"
                     "2.2.2.2:tcp(80)\n");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 7);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1.1.1.1" });
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT_UDP);
        checkStringList(rf.values, { "53" });
    }
}

TEST_F(RuleTextParserTest, lineBracketValues)
{
    RuleTextParser p("area(inet):udp(53):dir(out)");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 5);
}

TEST_F(RuleTextParserTest, lineIpPortSubList)
{
    RuleTextParser p("{1}\n2");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 3);
}

TEST_F(RuleTextParserTest, linesNotBracketValues)
{
    RuleTextParser p("!(1.1.1.1)\n!(2.2.2.2)");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 3);

    // Check Last IP
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_TRUE(rf.isNot);
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "2.2.2.2" });
    }
}

TEST_F(RuleTextParserTest, lineIpEqualValues)
{
    RuleTextParser p("1:=local_ip");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);

    // Check Local IP
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_TRUE(rf.equalValues);
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS);
        ASSERT_TRUE(rf.values.isEmpty());
    }
}

TEST_F(RuleTextParserTest, lineZonesAtLineEnd)
{
    RuleTextParser p("dir(in):!zones\n1.1.1.1");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 5);

    // Check Zones
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_TRUE(rf.isNot);
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ZONES);
    }

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[4];
        ASSERT_FALSE(rf.isNot);
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1.1.1.1" });
    }
}

TEST_F(RuleTextParserTest, lineZonesAtListEnd)
{
    RuleTextParser p("{dir(in):zones}:tcp(80)");

    ASSERT_TRUE(p.parse());

    const auto &ruleFilters = p.ruleFilters();

    ASSERT_EQ(ruleFilters.size(), 7);

    // Check Zones
    {
        const RuleFilter &rf = ruleFilters[5];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ZONES);
    }

    // Check TCP
    {
        const RuleFilter &rf = ruleFilters[6];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_PORT_TCP);
        checkStringList(rf.values, { "80" });
    }
}

TEST_F(RuleTextParserTest, linePortEqualValues)
{
    RuleTextParser p("tcp(21)=local_port:dir(in)");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 5);
}

TEST_F(RuleTextParserTest, lineVerticalLine)
{
    RuleTextParser p("1|2");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 3);
}
