#ifndef HOSTINFOCACHE_H
#define HOSTINFOCACHE_H

#include <QCache>
#include <QObject>

#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>

#include "hostinfo.h"

class HostInfoManager;

class HostInfoCache : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit HostInfoCache(QObject *parent = nullptr);
    ~HostInfoCache() override;

signals:
    void cacheChanged();

public slots:
    QString hostName(const QString &address);

    void clear();

private slots:
    void close();

    void handleFinishedLookup(const QString &address, const QString &hostName);

private:
    void emitCacheChanged();

private:
    HostInfoManager *m_manager = nullptr;

    QCache<QString, HostInfo> m_cache;

    TriggerTimer m_triggerTimer;
};

#endif // HOSTINFOCACHE_H
