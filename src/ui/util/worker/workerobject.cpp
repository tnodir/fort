#include "workerobject.h"

#include "workerjob.h"
#include "workermanager.h"

WorkerObject::WorkerObject(WorkerManager *manager) : m_manager(manager) { }

void WorkerObject::run()
{
    QThread::currentThread()->setPriority(priority());

    for (;;) {
        WorkerJobPtr job = manager()->dequeueJob();
        if (!job)
            break;

        doJob(*job);
    }

    manager()->workerFinished(this);
}

void WorkerObject::doJob(WorkerJob &job)
{
    job.doJob(*this);

    if (!manager()->aborted()) {
        job.reportResult(*this);
    }
}
