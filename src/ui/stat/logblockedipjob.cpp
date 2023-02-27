#include "logblockedipjob.h"

#include <util/worker/workerobject.h>

#include "statblockmanager.h"

LogBlockedIpJob::LogBlockedIpJob(qint64 unixTime) : WorkerJob(), m_unixTime(unixTime) { }

void LogBlockedIpJob::doJob(WorkerObject *worker)
{
    // log(static_cast<StatManager *>(worker->manager()));
}

void LogBlockedIpJob::reportResult(WorkerObject *worker)
{
    emitFinished(static_cast<StatBlockManager *>(worker->manager()));
}

void LogBlockedIpJob::emitFinished(StatBlockManager *manager)
{
    // emit manager->logFinished();
}
