#ifndef HOSTINFOCACHE_H
#define HOSTINFOCACHE_H

#include <QCache>
#include <QObject>
#include <QTimer>

#include "hostinfo.h"

QT_FORWARD_DECLARE_CLASS(HostInfoManager)

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
    void handleFinishedLookup(const QString &address,
                              const QString &hostName);

private:
    void emitCacheChanged();

private:
    HostInfoManager *m_manager;

    QCache<QString, HostInfo> m_cache;

    QTimer m_triggerTimer;
};

#endif // HOSTINFOCACHE_H
