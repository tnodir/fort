#ifndef APPINFOWORKER_H
#define APPINFOWORKER_H

#include <util/worker/workerobject.h>

class AppInfo;
class AppInfoManager;

class AppInfoWorker : public WorkerObject
{
public:
    explicit AppInfoWorker(AppInfoManager *manager);

    AppInfoManager *manager() const;

    void run() override;

protected:
    void doJob(WorkerJob *workerJob) override;
};

#endif // APPINFOWORKER_H
