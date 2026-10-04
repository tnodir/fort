#ifndef GROUP_H
#define GROUP_H

#include <QDateTime>
#include <QObject>

class Group
{
public:
    bool isFlagsEqual(const Group &o) const;
    bool isOptionsEqual(const Group &o) const;
    bool isNameEqual(const Group &o) const;

    QString menuLabel(const QString &periodName) const;

public:
    bool enabled : 1 = true;
    bool exclusive : 1 = false;
    bool periodEnabled : 1 = true;

    quint8 groupId = 0;
    quint8 periodId = 0;

    quint16 ruleId = 0;

    QString groupName;
    QString notes;

    QDateTime modTime;
};

#endif // GROUP_H
