#include "hostinfocache.h"

#include "hostinfomanager.h"

HostInfoCache::HostInfoCache(QObject *parent) :
    QObject(parent),
    m_manager(new HostInfoManager(this)),
    m_cache(1 * 1024 * 1024)
{
    connect(m_manager, &HostInfoManager::lookupFinished,
            this, &HostInfoCache::handleFinishedLookup);

    m_triggerTimer.setSingleShot(true);
    m_triggerTimer.setInterval(200);

    connect(&m_triggerTimer, &QTimer::timeout,
            this, &HostInfoCache::cacheChanged);
}

QString HostInfoCache::hostName(const QString &address)
{
    HostInfo *hostInfo = m_cache.object(address);

    if (hostInfo == nullptr) {
        hostInfo = new HostInfo();

        m_cache.insert(address, hostInfo, 4);
        m_manager->lookupHost(address);
    }

    return hostInfo->hostName;
}

void HostInfoCache::clear()
{
    m_manager->clear();
    m_cache.clear();

    emitCacheChanged();
}

void HostInfoCache::handleFinishedLookup(const QString &address,
                                         const QString &hostName)
{
    HostInfo *hostInfo = m_cache.take(address);
    if (hostInfo == nullptr)
        return;

    hostInfo->hostName = hostName;
    m_cache.insert(address, hostInfo, hostName.size());

    emitCacheChanged();
}

void HostInfoCache::emitCacheChanged()
{
    if (!m_triggerTimer.isActive()) {
        m_triggerTimer.start();
    }
}
