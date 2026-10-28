#include "timeperiod.h"

#include <QLocale>

#include <util/dateutil.h>

namespace {

bool parseTime(const QString &text, QString &time)
{
    const QTime t = DateUtil::parseTimeText(text);
    if (!t.isValid())
        return false;

    time = DateUtil::timeText(t);

    return true;
}

// "hh:mm-hh:mm"
bool parseTimeRange(const QString &text, TimePeriodInterval &interval)
{
    const QStringList times = text.split('-');
    if (times.size() != 2)
        return false;

    return parseTime(times[0], interval.timeFrom) && parseTime(times[1], interval.timeTo);
}

int prevDayOfWeek(int dayOfWeek)
{
    return (dayOfWeek == Qt::Monday) ? Qt::Sunday : (dayOfWeek - 1);
}

QString weekDaysRangeText(const QLocale &locale, int firstDay, int lastDay)
{
    const QString firstDayName = locale.dayName(firstDay, QLocale::ShortFormat);

    if (firstDay == lastDay)
        return firstDayName;

    const QString lastDayName = locale.dayName(lastDay, QLocale::ShortFormat);
    const QString sep = (lastDay == firstDay + 1) ? QString(", ") : QString(QChar(0x2013)); // –

    return firstDayName + sep + lastDayName;
}

}

bool TimePeriodInterval::hasWeekDay(int dayOfWeek) const
{
    return (weekDays & DateUtil::weekDayBit(dayOfWeek)) != 0;
}

bool TimePeriodInterval::isActive(int dayOfWeek, QTime time) const
{
    const QTime from = DateUtil::parseTime(timeFrom);
    const QTime to = DateUtil::parseTime(timeTo);

    const bool overMidnight = (from >= to);

    // Started today
    if (time >= from && hasWeekDay(dayOfWeek)) {
        return overMidnight || time < to;
    }

    // Started yesterday
    return overMidnight && isActiveFromPrevDay(dayOfWeek, time);
}

bool TimePeriodInterval::isActiveFromPrevDay(int dayOfWeek, QTime time) const
{
    const QTime to = DateUtil::parseTime(timeTo);

    return time < to && hasWeekDay(prevDayOfWeek(dayOfWeek));
}

QString TimePeriodInterval::weekDaysText() const
{
    const QLocale locale;

    QStringList list;

    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        if (!hasWeekDay(day))
            continue;

        const int firstDay = day;
        day = lastWeekDayOfRange(firstDay);

        list << weekDaysRangeText(locale, firstDay, day);
    }

    return list.join(", ");
}

QString TimePeriodInterval::label() const
{
    return weekDaysText() + QLatin1Char(' ') + DateUtil::formatPeriod(timeFrom, timeTo);
}

int TimePeriodInterval::lastWeekDayOfRange(int day) const
{
    while (day < Qt::Sunday && hasWeekDay(day + 1)) {
        ++day;
    }

    return day;
}

bool TimePeriod::isOptionsEqual(const TimePeriod &o) const
{
    return enabled == o.enabled && notes == o.notes && intervals == o.intervals;
}

bool TimePeriod::isNameEqual(const TimePeriod &o) const
{
    return name == o.name;
}

bool TimePeriod::isActive(const QDateTime &dateTime) const
{
    return !enabled || isActive(intervals, dateTime);
}

QString TimePeriod::intervalsText() const
{
    return intervalsText(intervals);
}

bool TimePeriod::isActive(const TimePeriodIntervals &intervals, const QDateTime &dateTime)
{
    const int dayOfWeek = dateTime.date().dayOfWeek();
    const QTime time = dateTime.time();

    return std::any_of(intervals.constBegin(), intervals.constEnd(),
            [&](const TimePeriodInterval &interval) { return interval.isActive(dayOfWeek, time); });
}

QString TimePeriod::intervalsText(const TimePeriodIntervals &intervals)
{
    QStringList list;

    for (const TimePeriodInterval &interval : intervals) {
        list << interval.label();
    }

    return list.join("; ");
}

bool TimePeriod::parseIntervals(const QString &text, TimePeriodIntervals &intervals)
{
    intervals.clear();

    TimePeriodInterval interval { .weekDays = 0 };

    for (const QString &part : text.split(',')) {
        const int timeIndex = part.indexOf('/');

        const quint8 weekDays = DateUtil::parseWeekDays(part.left(timeIndex));
        if (weekDays == 0)
            return false;

        interval.weekDays |= weekDays;

        if (timeIndex < 0)
            continue; // the days of the next part's time

        if (!parseTimeRange(part.mid(timeIndex + 1), interval))
            return false;

        intervals.append(interval);
        interval = { .weekDays = 0 };
    }

    // The whole days
    if (interval.weekDays != 0) {
        interval.timeFrom = interval.timeTo = "00:00";
        intervals.append(interval);
    }

    return !intervals.isEmpty();
}
