#include "hostinfomanager.h"

#include "hostinfojob.h"

HostInfoManager::HostInfoManager(QObject *parent) :
    WorkerManager(parent)
{
    setMaxWorkersCount(2);

    QSysInfo::machineHostName();  // Initialize ws2_32.dll
}

void HostInfoManager::lookupHost(const QString &address)
{
    enqueueJob(new HostInfoJob(address));
}

void HostInfoManager::handleWorkerResult(WorkerJob *workerJob)
{
    if (!aborted()) {
        auto job = static_cast<HostInfoJob *>(workerJob);

        emit lookupFinished(job->address(), job->hostName);
    }

    delete workerJob;
}
