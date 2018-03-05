#include "dateutil.h"

#include <QLocale>

DateUtil::DateUtil(QObject *parent) :
    QObject(parent)
{
}

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

    return getUnixHour(QDateTime(date).toSecsSinceEpoch());
}

qint32 DateUtil::getUnixMonth(qint64 unixTime, int monthStart)
{
    QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();

    if (date.day() < monthStart) {
        date = date.addMonths(-1);
    }

    return getUnixHour(QDateTime(QDate(date.year(), date.month(), 1))
                       .toSecsSinceEpoch());
}

qint32 DateUtil::addUnixMonths(qint32 unixHour, int months)
{
    const qint64 unixTime = DateUtil::toUnixTime(unixHour);

    return getUnixHour(QDateTime::fromSecsSinceEpoch(unixTime)
                       .addMonths(months)
                       .toSecsSinceEpoch());
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
    return formatDateTime(unixTime, "dd MMMM yyyy");
}

QString DateUtil::formatMonth(qint64 unixTime)
{
    return formatDateTime(unixTime, "MMMM yyyy");
}

QString DateUtil::formatDateTime(qint64 unixTime, const QString &format)
{
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(unixTime);
    return QLocale().toString(dt, format);
}
