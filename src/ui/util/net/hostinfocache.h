#ifndef HOSTINFOCACHE_H
#define HOSTINFOCACHE_H

#include <QCache>
#include <QObject>

#include "../triggertimer.h"
#include "hostinfo.h"

class HostInfoManager;

class HostInfoCache : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hostTrigger READ hostTrigger NOTIFY cacheChanged)

public:
    explicit HostInfoCache(QObject *parent = nullptr);

    bool hostTrigger() const { return true; }

signals:
    void cacheChanged();

public slots:
    QString hostName(const QString &address);

    void clear();

private slots:
    void handleFinishedLookup(const QString &address, const QString &hostName);

private:
    void emitCacheChanged();

private:
    HostInfoManager *m_manager = nullptr;

    QCache<QString, HostInfo> m_cache;

    TriggerTimer m_triggerTimer;
};

#endif // HOSTINFOCACHE_H
