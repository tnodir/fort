#ifndef APP_H
#define APP_H

#include <QDateTime>
#include <QObject>

class App
{
public:
    bool isEqual(const App &o) const;
    bool isFlagsEqual(const App &o) const;

public:
    bool isWildcard : 1 = false;
    bool useGroupPerm : 1 = true;
    bool applyChild : 1 = false;
    bool killChild : 1 = false;
    bool lanOnly : 1 = false;
    bool logBlocked : 1 = true;
    bool logConn : 1 = true;
    bool blocked : 1 = false;
    bool killProcess : 1 = false;
    bool alerted : 1 = false;

    int groupIndex = 0;

    qint64 appId = 0;

    QString appOriginPath;
    QString appPath;
    QString appName;

    QDateTime endTime;
    QDateTime creatTime;
};

#endif // APP_H
