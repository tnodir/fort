#include "appinfocache.h"

#include <QIcon>
#include <QImage>

#include "../iconcache.h"
#include "appinfomanager.h"

AppInfoCache::AppInfoCache(QObject *parent) : QObject(parent), m_cache(1000)
{
    m_triggerTimer.setSingleShot(true);
    m_triggerTimer.setInterval(200);

    connect(&m_triggerTimer, &QTimer::timeout, this, &AppInfoCache::cacheChanged);
}

void AppInfoCache::setManager(AppInfoManager *manager)
{
    Q_ASSERT(manager != nullptr);

    m_manager = manager;

    connect(m_manager, &AppInfoManager::lookupFinished, this, &AppInfoCache::handleFinishedLookup);
}

QImage AppInfoCache::appImage(const AppInfo &info) const
{
    return manager()->loadIconFromDb(info.iconId);
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
    } else if (!nullIconPath.isEmpty()) {
        pixmap = IconCache::file(nullIconPath);
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

    if (appInfo == nullptr) {
        appInfo = new AppInfo();

        m_cache.insert(appPath, appInfo, 1);
        lookupRequired = true;
    } else {
        lookupRequired = appInfo->isFileModified(appPath);
    }

    if (lookupRequired && m_manager != nullptr) {
        m_manager->lookupAppInfo(appPath);
    }

    return *appInfo;
}

void AppInfoCache::handleFinishedLookup(const QString &appPath, const AppInfo info)
{
    AppInfo *appInfo = m_cache.object(appPath);
    if (appInfo == nullptr)
        return;

    *appInfo = info;

    IconCache::remove(appPath); // invalidate cached icon

    emitCacheChanged();
}

void AppInfoCache::emitCacheChanged()
{
    if (!m_triggerTimer.isActive()) {
        m_triggerTimer.start();
    }
}
