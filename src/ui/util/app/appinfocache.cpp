#include "appinfocache.h"

#include "appinfomanager.h"

AppInfoCache::AppInfoCache(QObject *parent) :
    QObject(parent),
    m_manager(new AppInfoManager(this)),
    m_cache(1000)
{
    connect(m_manager, &AppInfoManager::lookupFinished,
            this, &AppInfoCache::handleFinishedLookup);

    m_triggerTimer.setSingleShot(true);
    m_triggerTimer.setInterval(200);

    connect(&m_triggerTimer, &QTimer::timeout,
            this, &AppInfoCache::cacheChanged);
}

AppInfo *AppInfoCache::appInfo(const QString &appPath)
{
    AppInfo *appInfo = m_cache.object(appPath);

    if (appInfo == nullptr) {
        appInfo = new AppInfo();

        m_cache.insert(appPath, appInfo, 1);
        m_manager->lookupApp(appPath);
    }

    return appInfo;
}

void AppInfoCache::handleFinishedLookup(const QString &appPath,
                                        const AppInfo info)
{
    AppInfo *appInfo = m_cache.object(appPath);
    if (appInfo == nullptr)
        return;

    *appInfo = info;

    emitCacheChanged();
}

void AppInfoCache::emitCacheChanged()
{
    if (!m_triggerTimer.isActive()) {
        m_triggerTimer.start();
    }
}
