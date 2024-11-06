#pragma once

#include <QDebug>

#include <googletest.h>

#include <util/conf/ruletextparser.h>

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
