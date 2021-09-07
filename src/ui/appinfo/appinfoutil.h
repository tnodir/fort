#ifndef APPINFOUTIL_H
#define APPINFOUTIL_H

#include <QImage>
#include <QObject>

class AppInfo;

namespace AppInfoUtil {

bool getInfo(const QString &appPath, AppInfo &appInfo);
QImage getIcon(const QString &appPath);

void initThread();
void doneThread();

bool fileExists(const QString &appPath);
QDateTime fileModTime(const QString &appPath);

bool openFolder(const QString &appPath);

}

#endif // APPINFOUTIL_H
