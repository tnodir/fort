#ifndef APPUTIL_H
#define APPUTIL_H

#include <QImage>
#include <QObject>

class AppInfo;

class AppUtil
{
public:
    static bool getInfo(const QString &appPath, AppInfo &appInfo);
    static QImage getIcon(const QString &appPath);

    static void initThread();
    static void doneThread();
};

#endif // APPUTIL_H
