#ifndef APPINFOCACHE_H
#define APPINFOCACHE_H

#include <QCache>
#include <QObject>

#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>

#include "appinfo.h"

class AppInfoCache : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit AppInfoCache(QObject *parent = nullptr);

    void setUp() override;
    void tearDown() override;

    QString appName(const QString &appPath);

    QPixmap appPixmap(const QString &appPath, const QString &nullIconPath = QString());
    QIcon appIcon(const QString &appPath, const QString &nullIconPath = QString());

    AppInfo appInfo(const QString &appPath);

signals:
    void cacheChanged();

private slots:
    void handleFinishedInfoLookup(const QString &appPath, const AppInfo &info);
    void handleFinishedIconLookup(const QString &appPath, const QImage &image);

private:
    void appInfoCached(const QString &appPath, AppInfo &info, bool &lookupRequired);

    void emitCacheChanged();

private:
    QCache<QString, AppInfo> m_cache;

    TriggerTimer m_triggerTimer;
};

#endif // APPINFOCACHE_H
