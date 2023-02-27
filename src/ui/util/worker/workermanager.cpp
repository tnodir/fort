#include "workermanager.h"

#include <QThreadPool>

#include "workerobject.h"

namespace {
constexpr unsigned long WORKER_TIMEOUT_MSEC = 5000;
}

WorkerManager::WorkerManager(QObject *parent) : QObject(parent) { }

WorkerManager::~WorkerManager()
{
    clear();
    abortWorkers();
}

void WorkerManager::setupWorker()
{
    const int workersCount = m_workers.size();

    if (workersCount != 0 && (workersCount >= maxWorkersCount() || m_jobQueue.isEmpty()))
        return;

    WorkerObject *worker = createWorker(); // autoDelete = true
    m_workers.append(worker);

    QThreadPool::globalInstance()->start(worker);
}

void WorkerManager::workerFinished(WorkerObject *worker)
{
    QMutexLocker locker(&m_mutex);

    m_workers.removeOne(worker);

    if (m_workers.isEmpty()) {
        m_waitCondition.wakeOne();
    }
}

WorkerObject *WorkerManager::createWorker()
{
    return new WorkerObject(this);
}

void WorkerManager::clear()
{
    QMutexLocker locker(&m_mutex);

    qDeleteAll(m_jobQueue);

    m_jobQueue.clear();
}

void WorkerManager::abortWorkers()
{
    QMutexLocker locker(&m_mutex);

    m_aborted = true;

    m_waitCondition.wakeAll();

    while (!m_workers.isEmpty()) {
        m_waitCondition.wait(&m_mutex);
    }
}

void WorkerManager::enqueueJob(WorkerJob *job)
{
    QMutexLocker locker(&m_mutex);

    setupWorker();

    m_jobQueue.enqueue(job);

    m_waitCondition.wakeOne();
}

WorkerJob *WorkerManager::dequeueJob()
{
    QMutexLocker locker(&m_mutex);

    while (!m_aborted && m_jobQueue.isEmpty()) {
        if (!m_waitCondition.wait(&m_mutex, WORKER_TIMEOUT_MSEC))
            break; // timed out
    }

    if (m_aborted || m_jobQueue.isEmpty())
        return nullptr;

    return m_jobQueue.dequeue();
}
