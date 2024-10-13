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

TEST_F(DateUtilTest, checkPeriod)
{
    const QTime x(15, 35);

    ASSERT_TRUE(DateUtil::isTimeInPeriod(x, { 0, 0 }, { 23, 0 }));
    ASSERT_TRUE(DateUtil::isTimeInPeriod(x, { 15, 0 }, { 16, 0 }));
    ASSERT_TRUE(DateUtil::isTimeInPeriod(x, { 15, 0 }, { 10, 0 }));
    ASSERT_FALSE(DateUtil::isTimeInPeriod(x, { 15, 0 }, { 15, 0 }));
    ASSERT_FALSE(DateUtil::isTimeInPeriod(x, { 0, 0 }, { 15, 0 }));
    ASSERT_FALSE(DateUtil::isTimeInPeriod(x, { 16, 0 }, { 15, 0 }));
    ASSERT_FALSE(DateUtil::isTimeInPeriod(x, { 24, 0 }, { 0, 0 }));
    ASSERT_FALSE(DateUtil::isTimeInPeriod(x, { 16, 0 }, { 14, 0 }));
    ASSERT_FALSE(DateUtil::isTimeInPeriod(x, { 16, 0 }, { 23, 0 }));

    ASSERT_TRUE(DateUtil::isTimeInPeriod(x, { 15, 35 }, { 15, 36 }));
    ASSERT_FALSE(DateUtil::isTimeInPeriod(x, { 15, 34 }, { 15, 35 }));
}
