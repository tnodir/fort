#ifndef DATEUTIL_H
#define DATEUTIL_H

#include <QDateTime>
#include <QObject>
#include <QString>

class DateUtil : public QObject
{
    Q_OBJECT

public:
    explicit DateUtil(QObject *parent = nullptr);

    static QDateTime now();

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
};

#endif // DATEUTIL_H
