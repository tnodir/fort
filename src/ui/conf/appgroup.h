#ifndef APPGROUP_H
#define APPGROUP_H

#include <QObject>
#include <QTime>

class AppGroup
{
public:
    bool isNameEqual(const AppGroup &o) const;
    bool isOptionsEqual(const AppGroup &o) const;

    QString menuLabel() const;

public:
    bool enabled : 1 = true;
    bool periodEnabled : 1 = false;

    quint8 groupId = 0;

    QString groupName;
    QString notes;

    // In format "hh:mm"
    QString periodFrom;
    QString periodTo;

    QDateTime modTime;
};

class AppGroupPeriod
{
public:
    void setupPeriodTimes(const QString &periodFrom, const QString &periodTo);

    bool isTimeInPeriod(QTime time) const;

public:
    quint8 groupId = 0;

    QTime periodFromTime;
    QTime periodToTime;
};

#endif // APPGROUP_H
