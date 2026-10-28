#pragma once

#include <googletest.h>

#include <conf/timeperiod.h>
#include <util/dateutil.h>

class TimePeriodTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void TimePeriodTest::SetUp() { }

void TimePeriodTest::TearDown() { }

TEST_F(TimePeriodTest, intervalIsActive)
{
    TimePeriodInterval interval;
    interval.weekDays = DateUtil::weekDayBit(Qt::Monday) | DateUtil::weekDayBit(Qt::Wednesday);
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
    interval.weekDays = DateUtil::weekDayBit(Qt::Monday) | DateUtil::weekDayBit(Qt::Sunday);
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
    interval.weekDays = DateUtil::weekDayBit(Qt::Saturday) | DateUtil::weekDayBit(Qt::Sunday);
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
    interval.weekDays = DateUtil::weekDayBit(Qt::Monday);
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

    interval.weekDays = DateUtil::weekDayBit(Qt::Monday) | DateUtil::weekDayBit(Qt::Tuesday)
            | DateUtil::weekDayBit(Qt::Wednesday) | DateUtil::weekDayBit(Qt::Saturday)
            | DateUtil::weekDayBit(Qt::Sunday);
    ASSERT_EQ(interval.weekDaysText(),
            dayName(Qt::Monday) + QChar(0x2013) + dayName(Qt::Wednesday) + ", "
                    + dayName(Qt::Saturday) + ", " + dayName(Qt::Sunday));

    interval.weekDays = DateUtil::weekDayBit(Qt::Thursday);
    ASSERT_EQ(interval.weekDaysText(), dayName(Qt::Thursday));

    interval.weekDays = 0;
    ASSERT_EQ(interval.weekDaysText(), QString());
}

TEST_F(TimePeriodTest, parseIntervals)
{
    TimePeriodIntervals intervals;

    ASSERT_TRUE(TimePeriod::parseIntervals("1-5/09:00-18:00,6,7/10:00-12:00", intervals));
    ASSERT_EQ(intervals.size(), 2);
    ASSERT_EQ(intervals[0].weekDays, 0x1F);
    ASSERT_EQ(intervals[0].timeFrom, "09:00");
    ASSERT_EQ(intervals[0].timeTo, "18:00");
    ASSERT_EQ(intervals[1].weekDays,
            DateUtil::weekDayBit(Qt::Saturday) | DateUtil::weekDayBit(Qt::Sunday));
    ASSERT_EQ(intervals[1].timeFrom, "10:00");
    ASSERT_EQ(intervals[1].timeTo, "12:00");

    ASSERT_TRUE(TimePeriod::parseIntervals("7/22:00-06:30,1,3-4", intervals));
    ASSERT_EQ(intervals.size(), 2);
    ASSERT_EQ(intervals[0].weekDays, DateUtil::weekDayBit(Qt::Sunday));
    ASSERT_EQ(intervals[0].timeFrom, "22:00");
    ASSERT_EQ(intervals[0].timeTo, "06:30");
    ASSERT_EQ(intervals[1].weekDays,
            DateUtil::weekDayBit(Qt::Monday) | DateUtil::weekDayBit(Qt::Wednesday)
                    | DateUtil::weekDayBit(Qt::Thursday));
    ASSERT_EQ(intervals[1].timeFrom, "00:00");
    ASSERT_EQ(intervals[1].timeTo, "00:00");
}

TEST_F(TimePeriodTest, parseIntervalsBad)
{
    const QStringList badTexts = { "", ",1", "1,", "0", "8", "x", "5-1", "1-2-3", "1/09:00",
        "1/09:00-", "1/24:00-10:00", "1/09:60-10:00", "1/09:00-10:00-11:00", "1/9:00-10:00x" };

    for (const QString &text : badTexts) {
        TimePeriodIntervals intervals;

        ASSERT_FALSE(TimePeriod::parseIntervals(text, intervals)) << text.toStdString();
    }
}
