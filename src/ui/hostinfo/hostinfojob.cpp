#include "hostinfojob.h"

#include <util/net/netutil.h>
#include <util/worker/workerobject.h>

#include "hostinfomanager.h"

HostInfoJob::HostInfoJob(const QString &address) : WorkerJob(address) { }

void HostInfoJob::doJob(WorkerObject & /*worker*/)
{
    m_hostName = NetUtil::getHostName(address());
}

void HostInfoJob::reportResult(WorkerObject &worker)
{
    emitFinished(static_cast<HostInfoManager *>(worker.manager()));
}

void HostInfoJob::emitFinished(HostInfoManager *manager)
{
    emit manager->lookupFinished(address(), m_hostName);
}
