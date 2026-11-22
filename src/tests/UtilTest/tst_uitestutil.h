#pragma once

#include <googletest.h>

#include <control/command/uitestutil.h>

class UiTestUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void UiTestUtilTest::SetUp() { }

void UiTestUtilTest::TearDown() { }

TEST_F(UiTestUtilTest, parseRange)
{
    UiTestRange range;

    ASSERT_TRUE(UiTestUtil::parseRange({}, range));
    ASSERT_EQ(range.first, 0);
    ASSERT_EQ(range.last, -1);

    ASSERT_TRUE(UiTestUtil::parseRange("5", range));
    ASSERT_EQ(range.first, 5);
    ASSERT_EQ(range.last, 5);

    ASSERT_TRUE(UiTestUtil::parseRange("3-", range));
    ASSERT_EQ(range.first, 3);
    ASSERT_EQ(range.last, -1);

    ASSERT_TRUE(UiTestUtil::parseRange("2-7", range));
    ASSERT_EQ(range.first, 2);
    ASSERT_EQ(range.last, 7);

    ASSERT_TRUE(UiTestUtil::parseRange("4-4", range));
    ASSERT_EQ(range.first, 4);
    ASSERT_EQ(range.last, 4);
}

TEST_F(UiTestUtilTest, parseRangeBad)
{
    const QStringList badTexts = { "-", "-3", "x", "x-", "7-2", "2-x", "2-3-4" };

    for (const QString &text : badTexts) {
        UiTestRange range;

        ASSERT_FALSE(UiTestUtil::parseRange(text, range)) << text.toStdString();
    }
}
