#include "dateutil.h"

#include <QDateTime>

DateUtil::DateUtil(QObject *parent) :
    QObject(parent)
{
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

qint32 DateUtil::getUnixMonth(qint64 unixTime)
{
    const QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();

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
    return formatDateTime(unixTime, "yyyy-MM-dd hh:mm:ss");
}

QString DateUtil::formatHour(qint64 unixTime)
{
    return formatDateTime(unixTime, "yyyy-MM-dd hh:00");
}

QString DateUtil::formatDay(qint64 unixTime)
{
    return formatDateTime(unixTime, "yyyy-MM-dd");
}

QString DateUtil::formatMonth(qint64 unixTime)
{
    return formatDateTime(unixTime, "yyyy-MM");
}

QString DateUtil::formatDateTime(qint64 unixTime, const QString &format)
{
    return QDateTime::fromSecsSinceEpoch(unixTime).toString(format);
}
