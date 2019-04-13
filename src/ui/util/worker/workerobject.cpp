#include "workerobject.h"

#include "workermanager.h"

WorkerObject::WorkerObject(WorkerManager *manager) :
    m_aborted(false),
    m_manager(manager)
{
}

void WorkerObject::run()
{
    while (!aborted()) {
        QString job;

        if (!manager()->dequeueJob(job))
            break;

        doJob(job);
    }

    manager()->workerFinished(this);
}
