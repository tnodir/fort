#include "hostinfojob.h"

#include "netutil.h"

HostInfoJob::HostInfoJob(const QString &address) :
    WorkerJob(address)
{
}

void HostInfoJob::doJob()
{
    hostName = NetUtil::getHostName(address());
}
