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

TEST_F(DateUtilTest, timeText)
{
    ASSERT_EQ(DateUtil::timeText(QTime(7, 5)), "07:05");

    ASSERT_EQ(DateUtil::parseTimeText("07:05"), QTime(7, 5));
    ASSERT_EQ(DateUtil::parseTimeText("23:59"), QTime(23, 59));

    const QStringList badTexts = { "", "x", "24:00", "09:60", "09:00x" };

    for (const QString &text : badTexts) {
        ASSERT_FALSE(DateUtil::parseTimeText(text).isValid()) << text.toStdString();
    }
}

TEST_F(DateUtilTest, parseWeekDays)
{
    ASSERT_EQ(DateUtil::parseWeekDays("3"), DateUtil::weekDayBit(Qt::Wednesday));
    ASSERT_EQ(DateUtil::parseWeekDays("1-5"), 0x1F);
    ASSERT_EQ(DateUtil::parseWeekDays("6-7"),
            DateUtil::weekDayBit(Qt::Saturday) | DateUtil::weekDayBit(Qt::Sunday));

    const QStringList badTexts = { "", "0", "8", "x", "5-1", "1-2-3", "1-", "-1" };

    for (const QString &text : badTexts) {
        ASSERT_EQ(DateUtil::parseWeekDays(text), 0) << text.toStdString();
    }
}

TEST_F(DateUtilTest, formatMinutes)
{
    ASSERT_EQ(DateUtil::formatMinutes(15), "15m");
    ASSERT_EQ(DateUtil::formatMinutes(6 * 60), "6h");
    ASSERT_EQ(DateUtil::formatMinutes(24 * 60), "1d");
    ASSERT_EQ(DateUtil::formatMinutes(25 * 60 + 30), "1d 1h 30m");
    ASSERT_EQ(DateUtil::formatMinutes(24 * 60 + 5), "1d 5m");
}
