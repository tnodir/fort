#ifndef DATEUTIL_H
#define DATEUTIL_H

#include <QDateTime>
#include <QLocale>
#include <QObject>
#include <QString>

class DateUtil
{
public:
    static QDateTime now();

    static QDateTime startOfDayUTC(const QDate &date);

    static qint64 getUnixTime();
    static qint64 toUnixTime(qint32 unixHour);

    static qint32 getUnixHour(qint64 unixTime);
    static qint32 getUnixDay(qint64 unixTime);
    static qint32 getUnixMonth(qint64 unixTime, int monthStart = 1);

    static qint32 addUnixMonths(qint32 unixHour, int months);

    static QString formatTime(qint64 unixTime);
    static QString formatHour(qint64 unixTime);
    static QString formatDay(qint64 unixTime);
    static QString formatMonth(qint64 unixTime);

    static QString formatDateTime(qint64 unixTime, const QString &format);

    static QString formatPeriod(const QString &from, const QString &to);

    static QString formatTime(quint8 hour, quint8 minute);
    static QString reformatTime(const QString &time);

    static QString localeDateTime(
            const QDateTime &dateTime, QLocale::FormatType format = QLocale::ShortFormat);

    static QTime currentTime();
    static QTime midnightTime();

    static QTime parseTime(const QString &time);

    static bool isTimeInPeriod(QTime time, QTime from, QTime to);
};

#endif // DATEUTIL_H
