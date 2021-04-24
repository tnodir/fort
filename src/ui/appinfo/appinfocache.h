#ifndef APPINFOCACHE_H
#define APPINFOCACHE_H

#include <QCache>
#include <QObject>

#include "../util/triggertimer.h"
#include "appinfo.h"

class AppIconProvider;
class AppInfoManager;

class AppInfoCache : public QObject
{
    Q_OBJECT

public:
    explicit AppInfoCache(QObject *parent = nullptr);

    AppInfoManager *manager() const { return m_manager; }
    void setManager(AppInfoManager *manager);

    QImage appImage(const AppInfo &info) const;
    QString appName(const QString &appPath);
    QIcon appIcon(const QString &appPath, const QString &nullIconPath = QString());

    AppInfo appInfo(const QString &appPath);

signals:
    void cacheChanged();

private slots:
    void handleFinishedLookup(const QString &appPath, const AppInfo &info);

private:
    void emitCacheChanged();

private:
    AppInfoManager *m_manager = nullptr;

    QCache<QString, AppInfo> m_cache;

    TriggerTimer m_triggerTimer;
};

#endif // APPINFOCACHE_H
