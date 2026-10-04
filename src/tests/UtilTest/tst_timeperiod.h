#pragma once

#include <googletest.h>

#include <conf/timeperiod.h>

class TimePeriodTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void TimePeriodTest::SetUp() { }

void TimePeriodTest::TearDown() { }

namespace {

inline constexpr quint8 weekDayBit(int dayOfWeek)
{
    return quint8(1u << (dayOfWeek - 1));
}

}

TEST_F(TimePeriodTest, intervalIsActive)
{
    TimePeriodInterval interval;
    interval.weekDays = weekDayBit(Qt::Monday) | weekDayBit(Qt::Wednesday);
    interval.timeFrom = "09:00";
    interval.timeTo = "18:00";

    ASSERT_FALSE(interval.isActive(Qt::Monday, QTime(8, 59)));
    ASSERT_TRUE(interval.isActive(Qt::Monday, QTime(9, 0)));
    ASSERT_TRUE(interval.isActive(Qt::Monday, QTime(17, 59)));
    ASSERT_FALSE(interval.isActive(Qt::Monday, QTime(18, 0)));

    ASSERT_FALSE(interval.isActive(Qt::Tuesday, QTime(12, 0)));
    ASSERT_TRUE(interval.isActive(Qt::Wednesday, QTime(12, 0)));
    ASSERT_FALSE(interval.isActive(Qt::Sunday, QTime(12, 0)));
}

TEST_F(TimePeriodTest, intervalOverMidnight)
{
    TimePeriodInterval interval;
    interval.weekDays = weekDayBit(Qt::Monday) | weekDayBit(Qt::Sunday);
    interval.timeFrom = "22:00";
    interval.timeTo = "06:00";

    ASSERT_TRUE(interval.isActive(Qt::Monday, QTime(23, 0)));
    ASSERT_TRUE(interval.isActive(Qt::Tuesday, QTime(5, 59))); // started on Monday
    ASSERT_FALSE(interval.isActive(Qt::Tuesday, QTime(6, 0)));
    ASSERT_FALSE(interval.isActive(Qt::Tuesday, QTime(23, 0)));

    ASSERT_TRUE(interval.isActive(Qt::Monday, QTime(1, 0))); // started on Sunday
    ASSERT_FALSE(interval.isActive(Qt::Monday, QTime(12, 0)));
    ASSERT_FALSE(interval.isActive(Qt::Sunday, QTime(1, 0)));
    ASSERT_TRUE(interval.isActive(Qt::Sunday, QTime(22, 0)));
}

TEST_F(TimePeriodTest, intervalWholeDay)
{
    TimePeriodInterval interval;
    interval.weekDays = weekDayBit(Qt::Saturday) | weekDayBit(Qt::Sunday);
    interval.timeFrom = "00:00";
    interval.timeTo = "00:00";

    ASSERT_FALSE(interval.isActive(Qt::Friday, QTime(23, 59)));
    ASSERT_TRUE(interval.isActive(Qt::Saturday, QTime(0, 0)));
    ASSERT_TRUE(interval.isActive(Qt::Sunday, QTime(23, 59)));
    ASSERT_FALSE(interval.isActive(Qt::Monday, QTime(0, 0)));

    // The whole 24 hours from the "from" time
    interval.timeFrom = interval.timeTo = "08:00";

    ASSERT_FALSE(interval.isActive(Qt::Saturday, QTime(7, 59)));
    ASSERT_TRUE(interval.isActive(Qt::Saturday, QTime(8, 0)));
    ASSERT_TRUE(interval.isActive(Qt::Monday, QTime(7, 59))); // started on Sunday
    ASSERT_FALSE(interval.isActive(Qt::Monday, QTime(8, 0)));
}

TEST_F(TimePeriodTest, periodIsActive)
{
    TimePeriod period;

    const QDateTime monday(QDate(2026, 10, 5), QTime(12, 30));
    ASSERT_EQ(monday.date().dayOfWeek(), Qt::Monday);

    ASSERT_FALSE(period.isActive(monday)); // no intervals

    TimePeriodInterval interval;
    interval.weekDays = weekDayBit(Qt::Monday);
    interval.timeFrom = "08:00";
    interval.timeTo = "12:00";

    period.intervals << interval;
    ASSERT_FALSE(period.isActive(monday));

    interval.timeFrom = "14:00";
    interval.timeTo = "17:00";

    period.intervals << interval;
    ASSERT_FALSE(period.isActive(monday));
    ASSERT_TRUE(period.isActive(monday.addSecs(2 * 3600)));
    ASSERT_FALSE(period.isActive(monday.addDays(1).addSecs(2 * 3600)));

    ASSERT_TRUE(period.isActive(monday.addSecs(-3600)));

    period.enabled = false;
    ASSERT_TRUE(period.isActive(monday)); // as if no Time Period
}

TEST_F(TimePeriodTest, weekDaysText)
{
    const QLocale locale;
    const auto dayName = [&](int day) { return locale.dayName(day, QLocale::ShortFormat); };

    TimePeriodInterval interval;
    ASSERT_EQ(interval.weekDaysText(), dayName(Qt::Monday) + QChar(0x2013) + dayName(Qt::Sunday));

    interval.weekDays = weekDayBit(Qt::Monday) | weekDayBit(Qt::Tuesday) | weekDayBit(Qt::Wednesday)
            | weekDayBit(Qt::Saturday) | weekDayBit(Qt::Sunday);
    ASSERT_EQ(interval.weekDaysText(),
            dayName(Qt::Monday) + QChar(0x2013) + dayName(Qt::Wednesday) + ", "
                    + dayName(Qt::Saturday) + ", " + dayName(Qt::Sunday));

    interval.weekDays = weekDayBit(Qt::Thursday);
    ASSERT_EQ(interval.weekDaysText(), dayName(Qt::Thursday));

    interval.weekDays = 0;
    ASSERT_EQ(interval.weekDaysText(), QString());
}
