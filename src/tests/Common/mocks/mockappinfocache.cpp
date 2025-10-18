#include "mockappinfocache.h"

MockAppInfoCache::MockAppInfoCache(QObject *parent) : AppInfoCache(parent) { }

QString MockAppInfoCache::appName(const QString &appPath)
{
    return appPath.right(9);
}
