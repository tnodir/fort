#include "dateutil.h"

#include <QLocale>

DateUtil::DateUtil(QObject *parent) : QObject(parent) { }

QDateTime DateUtil::now()
{
    return QDateTime::currentDateTime();
}

qint64 DateUtil::getUnixTime()
{
    return QDateTime::currentSecsSinceEpoch();
}

qint64 DateUtil::toUnixTime(qint32 unixHour)
{
    return qint64(unixHour) * 3600;
}

qint32 DateUtil::getUnixHour(qint64 unixTime)
{
    return qint32(unixTime / 3600);
}

qint32 DateUtil::getUnixDay(qint64 unixTime)
{
    const QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();
    const QDateTime dateTime = date.startOfDay(Qt::UTC);

    return getUnixHour(dateTime.toSecsSinceEpoch());
}

qint32 DateUtil::getUnixMonth(qint64 unixTime, int monthStart)
{
    QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();
    if (date.day() < monthStart) {
        date = date.addMonths(-1);
    }

    const QDate dateMonth = QDate(date.year(), date.month(), 1);
    const QDateTime dateTime = dateMonth.startOfDay(Qt::UTC);

    return getUnixHour(dateTime.toSecsSinceEpoch());
}

qint32 DateUtil::addUnixMonths(qint32 unixHour, int months)
{
    const qint64 unixTime = DateUtil::toUnixTime(unixHour);

    return getUnixHour(
            QDateTime::fromSecsSinceEpoch(unixTime).addMonths(months).toSecsSinceEpoch());
}

QString DateUtil::formatTime(qint64 unixTime)
{
    return formatDateTime(unixTime, "dd-MMM-yyyy hh:mm:ss");
}

QString DateUtil::formatHour(qint64 unixTime)
{
    return formatDateTime(unixTime, "dd-MMM-yyyy hh:00");
}

QString DateUtil::formatDay(qint64 unixTime)
{
    return formatDateTime(unixTime, "dd-MMM-yyyy");
}

QString DateUtil::formatMonth(qint64 unixTime)
{
    return formatDateTime(unixTime, "MMM-yyyy");
}

QString DateUtil::formatDateTime(qint64 unixTime, const QString &format)
{
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(unixTime);
    return QLocale::c().toString(dt, format);
}

QString DateUtil::formatPeriod(const QString &from, const QString &to)
{
    return QString::fromLatin1("[%1-%2)").arg(from, to);
}

QString DateUtil::formatTime(quint8 hour, quint8 minute)
{
    return QString::fromLatin1("%1:%2")
            .arg(hour, 2, 10, QLatin1Char('0'))
            .arg(minute, 2, 10, QLatin1Char('0'));
}

QString DateUtil::reformatTime(const QString &time)
{
    const int timeSize = time.size();
    if (timeSize == 5)
        return time;

    const quint8 hour = parseTimeHour(time);
    const quint8 minute = (timeSize < 4) ? 0 : parseTimeMinute(time);

    return formatTime(hour, minute);
}

void DateUtil::parseTime(const QString &time, quint8 &hour, quint8 &minute)
{
    hour = parseTimeHour(time);
    minute = parseTimeMinute(time);
}

quint8 DateUtil::parseTimeHour(const QString &period)
{
    return quint8(QStringView(period).left(2).toUInt());
}

quint8 DateUtil::parseTimeMinute(const QString &period)
{
    return quint8(QStringView(period).right(2).toUInt());
}

QString DateUtil::localeDateTime(const QDateTime &dateTime, QLocale::FormatType format)
{
    return QLocale().toString(dateTime, format);
}
