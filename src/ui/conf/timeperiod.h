#ifndef TIMEPERIOD_H
#define TIMEPERIOD_H

#include <QDateTime>
#include <QList>
#include <QObject>

inline constexpr quint8 TimePeriodAllWeekDays = 0x7F;

class TimePeriodInterval
{
public:
    bool operator==(const TimePeriodInterval &o) const = default;

    bool hasWeekDay(int dayOfWeek) const;

    // The interval is active from the "from" time of its week days till the "to" time,
    // possibly of the next day; the equal times mean the whole 24 hours
    bool isActive(int dayOfWeek, QTime time) const;

    QString weekDaysText() const;
    QString label() const;

private:
    bool isActiveFromPrevDay(int dayOfWeek, QTime time) const;

    int lastWeekDayOfRange(int day) const;

public:
    quint8 weekDays = TimePeriodAllWeekDays; // Monday = 1 << 0, ..., Sunday = 1 << 6

    // In format "hh:mm"
    QString timeFrom;
    QString timeTo;
};

using TimePeriodIntervals = QList<TimePeriodInterval>;

class TimePeriod
{
public:
    bool isOptionsEqual(const TimePeriod &o) const;
    bool isNameEqual(const TimePeriod &o) const;

    // The disabled Time Period is always active, as if it isn't set
    bool isActive(const QDateTime &dateTime) const;

    QString intervalsText() const;

    static bool isActive(const TimePeriodIntervals &intervals, const QDateTime &dateTime);

    static QString intervalsText(const TimePeriodIntervals &intervals);

public:
    bool enabled = true;

    quint8 periodId = 0;

    QString name;
    QString notes;

    TimePeriodIntervals intervals;

    QDateTime modTime;
};

#endif // TIMEPERIOD_H
