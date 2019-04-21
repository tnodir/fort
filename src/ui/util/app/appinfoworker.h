#ifndef APPINFOWORKER_H
#define APPINFOWORKER_H

#include "../worker/workerobject.h"

QT_FORWARD_DECLARE_CLASS(AppInfo)
QT_FORWARD_DECLARE_CLASS(AppInfoManager)

class AppInfoWorker : public WorkerObject
{
public:
    explicit AppInfoWorker(AppInfoManager *manager);

    AppInfoManager *manager() const;

protected:
    void doJob(WorkerJob *workerJob) override;
};

#endif // APPINFOWORKER_H
