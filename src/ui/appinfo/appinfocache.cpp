#include "appinfocache.h"

#include <QIcon>
#include <QImage>

#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

#include "appinfomanager.h"

AppInfoCache::AppInfoCache(QObject *parent) : QObject(parent), m_cache(1000)
{
    connect(&m_triggerTimer, &QTimer::timeout, this, &AppInfoCache::cacheChanged);
}

void AppInfoCache::setUp()
{
    connect(IoC<AppInfoManager>(), &AppInfoManager::lookupFinished, this,
            &AppInfoCache::handleFinishedLookup);
}

void AppInfoCache::tearDown()
{
    disconnect(IoC<AppInfoManager>());
}

QImage AppInfoCache::appImage(const AppInfo &info) const
{
    return IoC<AppInfoManager>()->loadIconFromDb(info.iconId);
}

QString AppInfoCache::appName(const QString &appPath)
{
    AppInfo appInfo = this->appInfo(appPath);
    if (!appInfo.isValid()) {
        IoC<AppInfoManager>()->loadInfoFromFs(appPath, appInfo);
    }
    return appInfo.fileDescription;
}

QIcon AppInfoCache::appIcon(const QString &appPath, const QString &nullIconPath)
{
    QPixmap pixmap;
    if (IconCache::find(appPath, &pixmap))
        return pixmap;

    const auto info = appInfo(appPath);
    const auto image = appImage(info);
    if (!image.isNull()) {
        pixmap = QPixmap::fromImage(image);
    } else {
        pixmap = IconCache::file(
                nullIconPath.isEmpty() ? ":/icons/application-window-96.png" : nullIconPath);
    }

    IconCache::insert(appPath, pixmap);

    return pixmap;
}

AppInfo AppInfoCache::appInfo(const QString &appPath)
{
    if (appPath.isEmpty())
        return AppInfo();

    AppInfo *appInfo = m_cache.object(appPath);
    bool lookupRequired = false;

    if (!appInfo) {
        appInfo = new AppInfo();

        m_cache.insert(appPath, appInfo, 1);
        lookupRequired = true;
    } else {
        lookupRequired = appInfo->isFileModified(appPath);
    }

    if (lookupRequired) {
        IoC<AppInfoManager>()->lookupAppInfo(appPath);
    }

    return *appInfo;
}

void AppInfoCache::handleFinishedLookup(const QString &appPath, const AppInfo &info)
{
    AppInfo *appInfo = m_cache.object(appPath);
    if (!appInfo)
        return;

    *appInfo = info;

    IconCache::remove(appPath); // invalidate cached icon

    emitCacheChanged();
}

void AppInfoCache::emitCacheChanged()
{
    m_triggerTimer.startTrigger();
}
