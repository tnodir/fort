#include "appinfocache.h"

#include <QImage>

#include "appinfomanager.h"

AppInfoCache::AppInfoCache(QObject *parent) :
    QObject(parent),
    m_cache(1000)
{
    m_triggerTimer.setSingleShot(true);
    m_triggerTimer.setInterval(200);

    connect(&m_triggerTimer, &QTimer::timeout, this, &AppInfoCache::cacheChanged);
}

void AppInfoCache::setManager(AppInfoManager *manager)
{
    Q_ASSERT(manager != nullptr);

    m_manager = manager;

    connect(m_manager, &AppInfoManager::lookupFinished,
            this, &AppInfoCache::handleFinishedLookup);
}

QImage AppInfoCache::appIcon(const AppInfo &info) const
{
    return manager()->loadIconFromDb(info.iconId);
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
