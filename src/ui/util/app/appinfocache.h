#ifndef APPINFOCACHE_H
#define APPINFOCACHE_H

#include <QCache>
#include <QObject>
#include <QTimer>

#include "appinfo.h"

QT_FORWARD_DECLARE_CLASS(AppIconProvider)
QT_FORWARD_DECLARE_CLASS(AppInfoManager)

class AppInfoCache : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool infoTrigger READ infoTrigger NOTIFY cacheChanged)

public:
    explicit AppInfoCache(QObject *parent = nullptr);

    bool infoTrigger() const { return true; }

    AppInfoManager *manager() const { return m_manager; }

signals:
    void cacheChanged();

public slots:
    AppInfo appInfo(const QString &appPath);

private slots:
    void handleFinishedLookup(const QString &appPath,
                              const AppInfo info);

private:
    void emitCacheChanged();

private:
    AppInfoManager *m_manager;

    QCache<QString, AppInfo> m_cache;

    QTimer m_triggerTimer;
};

#endif // APPINFOCACHE_H
