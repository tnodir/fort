#ifndef APPINFOMANAGER_H
#define APPINFOMANAGER_H

#include <QMutex>

#include <sqlite/sqlitetypes.h>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>
#include <util/worker/workermanager.h>

#include "appinfo.h"

class AppInfoManager : public WorkerManager, public IocService
{
    Q_OBJECT

public:
    explicit AppInfoManager(
            const QString &filePath, QObject *parent = nullptr, quint32 openFlags = 0);
    CLASS_DELETE_COPY_MOVE(AppInfoManager)

    SqliteDb *sqliteDb() const { return m_sqliteDb.data(); }

    void setUp() override;

    bool loadInfoFromFs(const QString &appPath, AppInfo &appInfo);
    QImage loadIconFromFs(const QString &appPath, const AppInfo &appInfo);

    bool loadInfoFromDb(const QString &appPath, AppInfo &appInfo);
    QImage loadIconFromDb(qint64 iconId);

    bool saveToDb(const QString &appPath, AppInfo &appInfo, const QImage &appIcon);

    void deleteAppInfo(const QString &appPath, const AppInfo &appInfo);
    void deleteOldApps(int limitCount = 0);

signals:
    void lookupInfoFinished(const QString &appPath, const AppInfo &appInfo);
    void lookupIconFinished(const QString &appPath, const QImage &image);

public slots:
    virtual void lookupAppInfo(const QString &appPath);
    void lookupAppIcon(const QString &appPath, qint64 iconId);

    void checkLookupInfoFinished(const QString &appPath);

protected:
    WorkerObject *createWorker() override;

    virtual void updateAppAccessTime(const QString &appPath);

private:
    bool setupDb();

    void saveAppIcon(const QImage &appIcon, QVariant &iconId, bool &ok);
    void saveAppInfo(
            const QString &appPath, const AppInfo &appInfo, const QVariant &iconId, bool &ok);

    void deleteExcessAppInfos();

    void getOldAppsAndIcons(
            QStringList &appPaths, QHash<qint64, int> &iconIds, int limitCount) const;
    bool deleteAppsAndIcons(const QStringList &appPaths, const QHash<qint64, int> &iconIds);

    void deleteIcons(const QHash<qint64, int> &iconIds, bool &ok);
    void deleteIcon(qint64 iconId, int deleteCount, bool &ok);

    void deleteApps(const QStringList &appPaths, bool &ok);
    void deleteApp(const QString &appPath, bool &ok);

private:
    SqliteDbPtr m_sqliteDb;
    QMutex m_mutex;
};

#endif // APPINFOMANAGER_H
