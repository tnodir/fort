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

    ASSERT_EQ(p.ruleFilters().size(), 3);

    // Check IP Values
    {
        const RuleFilter &rf = p.ruleFilters()[2];
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

TEST_F(RuleTextParserTest, lineSectionList)
{
    RuleTextParser p("ip(\n#1\n1.1.1.1/8\n#2\n2.2.2.2/16\n):{\ntcp(80)\n}");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 6);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_EQ(rf.type, FORT_RULE_FILTER_TYPE_ADDRESS);
        checkStringList(rf.values, { "1.1.1.1/8", "2.2.2.2/16" });
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[5];
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
