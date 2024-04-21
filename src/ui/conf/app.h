#ifndef APP_H
#define APP_H

#include <QDateTime>
#include <QObject>

class App
{
public:
    enum ScheduleAction : qint8 {
        ScheduleBlock = 0,
        ScheduleAllow,
        ScheduleRemove,
        ScheduleKillProcess,
    };

    bool isFlagsEqual(const App &o) const;
    bool isBaseFlagsEqual(const App &o) const;
    bool isExtraFlagsEqual(const App &o) const;
    bool isZonesEqual(const App &o) const;
    bool isPathsEqual(const App &o) const;
    bool isOptionsEqual(const App &o) const;
    bool isNameEqual(const App &o) const;

    bool isProcWild() const;

public:
    bool isWildcard : 1 = false;
    bool useGroupPerm : 1 = true;
    bool applyChild : 1 = false;
    bool killChild : 1 = false;
    bool lanOnly : 1 = false;
    bool parked : 1 = false;
    bool logBlocked : 1 = true;
    bool logConn : 1 = true;
    bool blocked : 1 = false;
    bool killProcess : 1 = false;
    bool alerted : 1 = false;

    qint8 scheduleAction = ScheduleBlock;

    qint8 groupIndex = 0; // "Main" app. group

    quint16 ruleId = 0;

    quint32 acceptZones = 0;
    quint32 rejectZones = 0;

    qint64 appId = 0;

    QString appOriginPath;
    QString appPath;
    QString appName;
    QString notes;
    QString ruleName; // transient

    QDateTime scheduleTime;
    QDateTime creatTime;
};

#endif // APP_H
