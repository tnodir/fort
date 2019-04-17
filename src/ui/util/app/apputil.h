#ifndef APPUTIL_H
#define APPUTIL_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(AppInfo)

class AppUtil
{
public:
    static bool getInfo(const QString &appPath, AppInfo &appInfo);
};

#endif // APPUTIL_H
