#include "hostinfocache.h"

#include "hostinfomanager.h"

HostInfoCache::HostInfoCache(QObject *parent) :
    QObject(parent), m_manager(new HostInfoManager(this)), m_cache(1000)
{
    connect(m_manager, &HostInfoManager::lookupFinished, this,
            &HostInfoCache::handleFinishedLookup);

    connect(&m_triggerTimer, &QTimer::timeout, this, &HostInfoCache::cacheChanged);
}

HostInfoCache::~HostInfoCache()
{
    clear();
    close();
}

QString HostInfoCache::hostName(const QString &address)
{
    HostInfo *hostInfo = m_cache.object(address);

    if (hostInfo) {
        return hostInfo->hostName;
    }

    hostInfo = new HostInfo();

    m_cache.insert(address, hostInfo, 1);
    /* hostInfo may be deleted */

    m_manager->lookupHost(address);

    return {};
}

void HostInfoCache::clear()
{
    m_manager->clear();
    m_cache.clear();

    emitCacheChanged();
}

void HostInfoCache::close()
{
    m_manager->abortWorkers();
}

void HostInfoCache::handleFinishedLookup(const QString &address, const QString &hostName)
{
    HostInfo *hostInfo = m_cache.object(address);
    if (!hostInfo)
        return;

    hostInfo->hostName = hostName;

    emitCacheChanged();
}

void HostInfoCache::emitCacheChanged()
{
    m_triggerTimer.startTrigger();
}
