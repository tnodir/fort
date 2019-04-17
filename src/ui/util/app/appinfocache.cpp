#include "appinfocache.h"

#include <QLoggingCategory>

#include "../fileutil.h"
#include "appinfomanager.h"
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

Q_DECLARE_LOGGING_CATEGORY(CLOG_APPINFOCACHE)
Q_LOGGING_CATEGORY(CLOG_APPINFOCACHE, "fort.appInfoCache")

#define DATABASE_USER_VERSION   1

AppInfoCache::AppInfoCache(QObject *parent) :
    QObject(parent),
    m_manager(new AppInfoManager(this)),
    m_sqliteDb(new SqliteDb()),
    m_cache(1000)
{
    connect(m_manager, &AppInfoManager::lookupFinished,
            this, &AppInfoCache::handleFinishedLookup);

    m_triggerTimer.setSingleShot(true);
    m_triggerTimer.setInterval(200);

    connect(&m_triggerTimer, &QTimer::timeout,
            this, &AppInfoCache::cacheChanged);

    setupDb();
}

AppInfoCache::~AppInfoCache()
{
    delete m_sqliteDb;
}

void AppInfoCache::setupDb()
{
    const QString filePath = FileUtil::appCacheLocation() + "/appinfocache.db";

    if (!m_sqliteDb->open(filePath)) {
        qCritical(CLOG_APPINFOCACHE()) << "File open error:"
                                       << filePath
                                       << m_sqliteDb->errorMessage();
        return;
    }

    if (!m_sqliteDb->migrate(":/appinfocache/migrations", DATABASE_USER_VERSION)) {
        qCritical(CLOG_APPINFOCACHE()) << "Migration error" << filePath;
        return;
    }
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
