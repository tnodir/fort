#ifndef APP_H
#define APP_H

#include <QDateTime>
#include <QObject>

class App
{
public:
    bool useGroupPerm = true;
    bool applyChild = false;
    bool lanOnly = false;
    bool logBlocked = true;
    bool logConn = true;
    bool blocked = false;
    bool alerted = false;

    int groupIndex = 0;

    qint64 appId = 0;

    QString appPath;
    QString appName;

    QDateTime endTime;
    QDateTime creatTime;
};

#endif // APP_H
