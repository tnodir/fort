#ifndef APPINFOWORKER_H
#define APPINFOWORKER_H

#include <util/worker/workerobject.h>

class AppInfoManager;

class AppInfoWorker : public WorkerObject
{
public:
    explicit AppInfoWorker(AppInfoManager *manager);

    QThread::Priority priority() const override { return QThread::LowPriority; }

    QString workerName() const override { return "AppInfoWorker"; }

    void run() override;
};

#endif // APPINFOWORKER_H
