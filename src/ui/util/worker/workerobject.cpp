#include "workerobject.h"

#include "workerjob.h"
#include "workermanager.h"

#include <util/osutil.h>

WorkerObject::WorkerObject(WorkerManager *manager) : m_manager(manager) { }

QString WorkerObject::workerName() const
{
    return manager()->workerName();
}

void WorkerObject::run()
{
    QThread::currentThread()->setPriority(priority());

    OsUtil::setCurrentThreadName(workerName());

    doJobs();

    manager()->workerFinished(this);
}

void WorkerObject::doJobs()
{
    for (;;) {
        WorkerJobPtr job = manager()->dequeueJob();
        if (!job)
            break;

        doJob(*job);
    }
}

void WorkerObject::doJob(WorkerJob &job)
{
    job.doJob(manager());

    if (!manager()->aborted()) {
        job.reportResult(manager());
    }
}
