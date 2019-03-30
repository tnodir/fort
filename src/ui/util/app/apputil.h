#ifndef APPUTIL_H
#define APPUTIL_H

#include <QObject>
#include <QPixmap>

typedef struct AppInfo {
    QString fileDescription;
    QString companyName;
    QString productName;
    QString productVersion;
} AppInfo;

class AppUtil
{
public:
    static QPixmap getIcon(const QString &appPath);
    static bool getInfo(const QString &appPath, AppInfo &appInfo);
};

#endif // APPUTIL_H
