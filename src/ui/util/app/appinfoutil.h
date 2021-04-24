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

}

#endif // APPINFOUTIL_H
