#ifndef APPINFOMANAGER_H
#define APPINFOMANAGER_H

#include <QMutex>

#include "../worker/workermanager.h"
#include "appinfo.h"

QT_FORWARD_DECLARE_CLASS(SqliteDb)

class AppInfoManager : public WorkerManager
{
    Q_OBJECT

public:
    explicit AppInfoManager(QObject *parent = nullptr);
    ~AppInfoManager() override;

    bool loadInfoFromFs(const QString &appPath, AppInfo &appInfo);
    QImage loadIconFromFs(const QString &appPath);

    bool loadInfoFromDb(const QString &appPath, AppInfo &appInfo);
    QImage loadIconFromDb(qint64 iconId);

    bool saveToDb(const QString &appPath, AppInfo &appInfo,
                  const QImage &appIcon);

signals:
    void lookupFinished(const QString &appPath, const AppInfo appInfo);

public slots:
    void lookupAppInfo(const QString &appPath);

    void handleWorkerResult(WorkerJob *workerJob) override;

protected:
    WorkerObject *createWorker() override;

private:
    void setupDb();

    void shrinkDb(int excessCount);

private:
    SqliteDb *m_sqliteDb;
    QMutex m_mutex;
};

#endif // APPINFOMANAGER_H
