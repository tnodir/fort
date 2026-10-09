#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <util/dateutil.h>

class DateUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void DateUtilTest::SetUp() { }

void DateUtilTest::TearDown() { }

TEST_F(DateUtilTest, formatMinutes)
{
    ASSERT_EQ(DateUtil::formatMinutes(15), "15m");
    ASSERT_EQ(DateUtil::formatMinutes(6 * 60), "6h");
    ASSERT_EQ(DateUtil::formatMinutes(24 * 60), "1d");
    ASSERT_EQ(DateUtil::formatMinutes(25 * 60 + 30), "1d 1h 30m");
    ASSERT_EQ(DateUtil::formatMinutes(24 * 60 + 5), "1d 5m");
}
