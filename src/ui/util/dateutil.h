#ifndef DATEUTIL_H
#define DATEUTIL_H

#include <QObject>

class DateUtil : public QObject
{
    Q_OBJECT

public:
    explicit DateUtil(QObject *parent = nullptr);

    static qint64 getUnixTime();
    static qint64 toUnixTime(qint32 unixHour);

    static qint32 getUnixHour(qint64 unixTime);
    static qint32 getUnixDay(qint64 unixTime);
    static qint32 getUnixMonth(qint64 unixTime);
};

#endif // DATEUTIL_H
