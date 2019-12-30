#ifndef APPUTIL_H
#define APPUTIL_H

#include <QDateTime>
#include <QObject>
#include <QImage>

QT_FORWARD_DECLARE_CLASS(AppInfo)

class AppUtil
{
public:
    static bool getInfo(const QString &appPath, AppInfo &appInfo);
    static QImage getIcon(const QString &appPath);
    static QDateTime getModTime(const QString &appPath);

    static void initThread();
    static void doneThread();
};

#endif // APPUTIL_H
