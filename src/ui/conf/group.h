#ifndef GROUP_H
#define GROUP_H

#include <QObject>
#include <QTime>

class Group
{
public:
    bool isFlagsEqual(const Group &o) const;
    bool isOptionsEqual(const Group &o) const;
    bool isNameEqual(const Group &o) const;

    QString menuLabel() const;

public:
    bool enabled : 1 = true;
    bool exclusive : 1 = false;
    bool periodEnabled : 1 = false;

    quint8 groupId = 0;

    quint16 ruleId = 0;

    QString groupName;
    QString notes;

    // In format "hh:mm"
    QString periodFrom;
    QString periodTo;

    QDateTime modTime;
};

class GroupPeriod
{
public:
    void setupPeriodTimes(const QString &periodFrom, const QString &periodTo);

    bool isTimeInPeriod(QTime time) const;

public:
    quint8 groupId = 0;

    QTime periodFromTime;
    QTime periodToTime;
};

#endif // GROUP_H
