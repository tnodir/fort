#pragma once

#include <QDebug>

#include <googletest.h>

#include <util/conf/ruletextparser.h>

namespace {

bool compareStringList(const StringViewList &l1, const QStringList &l2)
{
    if (l1.size() != l2.size())
        return false;

    for (int i = 0; i < l1.size(); ++i) {
        if (l1[i] != l2[i])
            return false;
    }

    return true;
}

}

class RuleTextParserTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void RuleTextParserTest::SetUp() { }

void RuleTextParserTest::TearDown() { }

TEST_F(RuleTextParserTest, emptyList)
{
    RuleTextParser p("{}");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 0);
}

TEST_F(RuleTextParserTest, lineIpPort)
{
    RuleTextParser p("1.1.1.1:53");

    ASSERT_TRUE(p.parse());

    ASSERT_EQ(p.ruleFilters().size(), 4);

    // Check IP
    {
        const RuleFilter &rf = p.ruleFilters()[2];
        ASSERT_TRUE(compareStringList(rf.values, { "1.1.1.1" }));
    }

    // Check Port
    {
        const RuleFilter &rf = p.ruleFilters()[3];
        ASSERT_TRUE(compareStringList(rf.values, { "53" }));
    }
}
