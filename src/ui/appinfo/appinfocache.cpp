#include "appinfocache.h"

#include <QIcon>
#include <QImage>

#include <fortglobal.h>
#include <util/iconcache.h>

#include "appinfomanager.h"

using namespace Fort;

AppInfoCache::AppInfoCache(QObject *parent) : QObject(parent), m_cache(1000)
{
    connect(&m_triggerTimer, &QTimer::timeout, this, &AppInfoCache::cacheChanged);
}

void AppInfoCache::setUp()
{
    auto appInfoManager = Fort::dependency<AppInfoManager>();

    connect(appInfoManager, &AppInfoManager::lookupInfoFinished, this,
            &AppInfoCache::handleFinishedInfoLookup);
    connect(appInfoManager, &AppInfoManager::lookupIconFinished, this,
            &AppInfoCache::handleFinishedIconLookup);
}

void AppInfoCache::tearDown()
{
    appInfoManager()->disconnect(this);
}

QString AppInfoCache::appName(const QString &appPath)
{
    AppInfo appInfo = this->appInfo(appPath);
    if (!appInfo.isValid()) {
        appInfoManager()->loadInfoFromFs(appPath, appInfo);
    }
    return appInfo.fileDescription;
}

QIcon AppInfoCache::appIcon(const QString &appPath, const QString &nullIconPath)
{
    QIcon icon;
    if (IconCache::find(appPath, icon))
        return icon;

    const auto info = appInfo(appPath);
    if (info.isValid()) {
        appInfoManager()->lookupAppIcon(appPath, info.iconId);
    }

    icon = IconCache::icon(
            !nullIconPath.isEmpty() ? nullIconPath : ":/icons/application-window-96.png");

    IconCache::insert(appPath, icon);

    return icon;
}

AppInfo AppInfoCache::appInfo(const QString &appPath)
{
    if (appPath.isEmpty())
        return {};

    AppInfo appInfo;
    bool lookupRequired;

    appInfoCached(appPath, appInfo, lookupRequired);

    if (!lookupRequired) {
        lookupRequired = appInfo.isValid() && appInfo.checkFileModified(appPath);
    }

    if (lookupRequired && appInfo.fileExists) {
        appInfoManager()->lookupAppInfo(appPath);
    }

    return appInfo;
}

void AppInfoCache::handleFinishedInfoLookup(const QString &appPath, const AppInfo &info)
{
    AppInfo *appInfo = m_cache.object(appPath);
    if (!appInfo)
        return;

    *appInfo = info;

    IconCache::remove(appPath); // invalidate cached icon

    emitCacheChanged();
}

void AppInfoCache::handleFinishedIconLookup(const QString &appPath, const QImage &image)
{
    if (image.isNull())
        return;

    const QPixmap pixmap = QPixmap::fromImage(image);

    IconCache::insert(appPath, pixmap); // update cached icon

    emitCacheChanged();
}

void AppInfoCache::appInfoCached(const QString &appPath, AppInfo &info, bool &lookupRequired)
{
    AppInfo *cachedInfo = m_cache.object(appPath);

    if (cachedInfo) {
        lookupRequired = false;

        info = *cachedInfo;
    } else {
        lookupRequired = !appInfoManager()->loadInfoFromDb(appPath, info);

        cachedInfo = new AppInfo();

        if (!lookupRequired) {
            *cachedInfo = info;
        }

        m_cache.insert(appPath, cachedInfo, /*cost=*/1);
        /* cachedInfo may be deleted */
    }
}

void AppInfoCache::emitCacheChanged()
{
    m_triggerTimer.startTrigger();
}
