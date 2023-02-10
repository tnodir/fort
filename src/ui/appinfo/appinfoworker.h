#ifndef APPINFOWORKER_H
#define APPINFOWORKER_H

#include <util/worker/workerobject.h>

class AppIconJob;
class AppInfo;
class AppInfoJob;
class AppInfoManager;

class AppInfoWorker : public WorkerObject
{
public:
    explicit AppInfoWorker(AppInfoManager *manager);

    AppInfoManager *manager() const;

    QThread::Priority priority() const override { return QThread::LowPriority; }

    void run() override;

protected:
    void doJob(WorkerJob *workerJob) override;

private:
    void loadAppInfo(AppInfoJob *job, const QString &appPath);
    void loadAppIcon(AppIconJob *job, const QString &appPath);
};

#endif // APPINFOWORKER_H
