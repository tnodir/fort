#include "workermanager.h"

#include <QThreadPool>

#include "workerjob.h"
#include "workerobject.h"

namespace {
constexpr unsigned long WORKER_TIMEOUT_MSEC = 5000;
}

WorkerManager::WorkerManager(QObject *parent) : QObject(parent) { }

WorkerManager::~WorkerManager()
{
    abortWorkers();
}

void WorkerManager::setupWorker()
{
    if (!checkNewWorkerNeeded())
        return;

    WorkerObject *worker = createWorker(); // autoDelete = true
    m_workers.append(worker);

    QThreadPool::globalInstance()->start(worker);
}

bool WorkerManager::checkNewWorkerNeeded() const
{
    const int workersCount = m_workers.size();

    if (workersCount == 0)
        return true;

    return workersCount < maxWorkersCount() && !m_jobQueue.isEmpty();
}

void WorkerManager::clearJobQueue()
{
    m_jobQueue.clear();
}

void WorkerManager::workerFinished(WorkerObject *worker)
{
    QMutexLocker locker(&m_mutex);

    m_workers.removeOne(worker);

    if (m_workers.isEmpty() && aborted()) {
        m_abortWaitCondition.wakeOne();
    }
}

WorkerObject *WorkerManager::createWorker()
{
    return new WorkerObject(this);
}

int WorkerManager::jobCount() const
{
    QMutexLocker locker(&m_mutex);

    return m_jobQueue.size();
}

bool WorkerManager::mergeJob(WorkerJobPtr job)
{
    if (!canMergeJobs() || m_jobQueue.isEmpty())
        return false;

    return m_jobQueue.last()->mergeJob(*job);
}

void WorkerManager::clear()
{
    QMutexLocker locker(&m_mutex);

    clearJobQueue();
}

void WorkerManager::abortWorkers()
{
    QMutexLocker locker(&m_mutex);

    clearJobQueue();

    m_aborted = true;

    m_jobWaitCondition.wakeAll();

    while (!m_workers.isEmpty()) {
        m_abortWaitCondition.wait(&m_mutex);
    }
}

void WorkerManager::enqueueJob(WorkerJobPtr job)
{
    QMutexLocker locker(&m_mutex);

    if (aborted())
        return;

    setupWorker();

    if (mergeJob(job))
        return;

    m_jobQueue.enqueue(job);

    m_jobWaitCondition.wakeOne();
}

WorkerJobPtr WorkerManager::dequeueJob()
{
    QMutexLocker locker(&m_mutex);

    while (!aborted() && m_jobQueue.isEmpty()) {
        if (!m_jobWaitCondition.wait(&m_mutex, WORKER_TIMEOUT_MSEC))
            break; // timed out
    }

    if (aborted() || m_jobQueue.isEmpty())
        return nullptr;

    return m_jobQueue.dequeue();
}
