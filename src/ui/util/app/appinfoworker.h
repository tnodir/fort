#ifndef APPINFOWORKER_H
#define APPINFOWORKER_H

#include "../worker/workerobject.h"

QT_FORWARD_DECLARE_CLASS(AppInfo)
QT_FORWARD_DECLARE_CLASS(AppInfoManager)
QT_FORWARD_DECLARE_CLASS(SqliteDb)

class AppInfoWorker : public WorkerObject
{
public:
    explicit AppInfoWorker(AppInfoManager *manager);
    ~AppInfoWorker() override;

protected:
    void doJob(const QString &appPath) override;

private:
    void setupDb();

    bool loadFromFs(const QString &appPath, AppInfo &appInfo);

    bool loadFromDb(const QString &appPath, AppInfo &appInfo);
    bool saveToDb(const QString &appPath, const AppInfo &appInfo);
    void shrinkDb(int excessCount);

private:
    SqliteDb *m_sqliteDb;
};

#endif // APPINFOWORKER_H
