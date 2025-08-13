#include "hostinfojob.h"

#include <util/net/netutil.h>
#include <util/worker/workerobject.h>

#include "hostinfomanager.h"

HostInfoJob::HostInfoJob(const QString &address) : WorkerJob(address) { }

void HostInfoJob::doJob(WorkerManager * /*manager*/)
{
    m_hostName = NetUtil::getHostName(address());
}

void HostInfoJob::reportResult(WorkerManager *manager)
{
    emitFinished(static_cast<HostInfoManager *>(manager));
}

void HostInfoJob::emitFinished(HostInfoManager *manager)
{
    emit manager->lookupFinished(address(), m_hostName);
}
