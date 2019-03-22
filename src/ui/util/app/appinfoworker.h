#ifndef APPINFOWORKER_H
#define APPINFOWORKER_H

#include "../worker/workerobject.h"

QT_FORWARD_DECLARE_CLASS(AppInfoManager)

class AppInfoWorker : public WorkerObject
{
public:
    explicit AppInfoWorker(AppInfoManager *manager);

protected:
    void doJob(const QString &appPath) override;
};

#endif // APPINFOWORKER_H
