#include "hostinfocache.h"

#include "hostinfo.h"

HostInfoCache::HostInfoCache(QObject *parent) :
    QObject(parent),
    m_hostInfo(new HostInfo(this))
{
    connect(m_hostInfo, &HostInfo::lookupFinished,
            this, &HostInfoCache::handleFinishedLookup);
}

QString HostInfoCache::hostName(const QString &address)
{
    if (!m_cache.contains(address)) {
        m_cache.insert(address, QString());
        m_hostInfo->lookupHost(address);
        return QString();
    }

    return m_cache.value(address);
}

void HostInfoCache::clear()
{
    m_hostInfo->clear();
    m_cache.clear();

    emitCacheChanged();
}

void HostInfoCache::handleFinishedLookup(const QString &address,
                                         const QString &hostName)
{
    m_cache.insert(address, hostName);

    emitCacheChanged();
}

void HostInfoCache::emitCacheChanged()
{
    emit cacheChanged();
}
