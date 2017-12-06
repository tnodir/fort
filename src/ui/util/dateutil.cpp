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
