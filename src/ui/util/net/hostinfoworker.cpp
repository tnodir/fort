#include "hostinfoworker.h"

#include "hostinfomanager.h"
#include "netutil.h"

HostInfoWorker::HostInfoWorker(HostInfoManager *manager) :
    WorkerObject(manager)
{
}

void HostInfoWorker::doJob(const QString &address)
{
    const QString hostName = NetUtil::getHostName(address);

    if (aborted()) return;

    manager()->handleWorkerResult(address, hostName);
}
