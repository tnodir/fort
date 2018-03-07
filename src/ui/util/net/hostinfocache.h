#ifndef HOSTINFOCACHE_H
#define HOSTINFOCACHE_H

#include <QObject>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(HostInfo)

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
    HostInfo *m_hostInfo;

    QHash<QString, QString> m_cache;
};

#endif // HOSTINFOCACHE_H
