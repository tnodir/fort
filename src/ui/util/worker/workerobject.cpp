#include "workerobject.h"

#include "workerjob.h"
#include "workermanager.h"

WorkerObject::WorkerObject(WorkerManager *manager) : m_manager(manager) { }

void WorkerObject::run()
{
    for (;;) {
        WorkerJob *job = manager()->dequeueJob();
        if (!job)
            break;

        doJob(job);
    }

    manager()->workerFinished(this);
}

void WorkerObject::doJob(WorkerJob *job)
{
    job->doJob();

    manager()->handleWorkerResult(job);
}
