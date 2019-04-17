#ifndef APPINFOCACHE_H
#define APPINFOCACHE_H

#include <QCache>
#include <QObject>
#include <QTimer>

#include "appinfo.h"

QT_FORWARD_DECLARE_CLASS(AppInfoManager)
QT_FORWARD_DECLARE_CLASS(SqliteDb)

class AppInfoCache : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool infoTrigger READ infoTrigger NOTIFY cacheChanged)

public:
    explicit AppInfoCache(QObject *parent = nullptr);
    ~AppInfoCache() override;

    bool infoTrigger() const { return true; }

signals:
    void cacheChanged();

public slots:
    AppInfo *appInfo(const QString &appPath);

private slots:
    void handleFinishedLookup(const QString &appPath,
                              const AppInfo info);

private:
    void setupDb();

    void emitCacheChanged();

private:
    AppInfoManager *m_manager;

    SqliteDb *m_sqliteDb;

    QCache<QString, AppInfo> m_cache;

    QTimer m_triggerTimer;
};

#endif // APPINFOCACHE_H
