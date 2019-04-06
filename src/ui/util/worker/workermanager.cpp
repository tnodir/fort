#include "workermanager.h"

#include <QThreadPool>

#include "workerobject.h"

namespace {
const unsigned long WORKER_TIMEOUT_MSEC = 15000;
}

WorkerManager::WorkerManager(QObject *parent) :
    QObject(parent),
    m_aborted(false),
    m_maxWorkersCount(0)
{
}

WorkerManager::~WorkerManager()
{
    abort();
}

void WorkerManager::setupWorker()
{
    const int workersCount = m_workers.size();

    if (workersCount != 0
            && (workersCount >= maxWorkersCount()
                || m_queue.isEmpty()))
        return;

    WorkerObject *worker = createWorker();  // autoDelete = true
    m_workers.append(worker);

    QThreadPool::globalInstance()->start(worker);
}

void WorkerManager::workerFinished(WorkerObject *worker)
{
    QMutexLocker locker(&m_mutex);

    m_workers.removeOne(worker);
}

void WorkerManager::clear()
{
    QMutexLocker locker(&m_mutex);

    m_queue.clear();
}

void WorkerManager::abort()
{
    QMutexLocker locker(&m_mutex);

    abortWorkers();

    m_aborted = true;

    m_waitCondition.wakeAll();
}

void WorkerManager::abortWorkers()
{
    for (WorkerObject *worker : m_workers) {
        worker->abort();
    }

    m_workers.clear();
}

void WorkerManager::enqueueJob(const QString &job)
{
    QMutexLocker locker(&m_mutex);

    setupWorker();

    m_queue.enqueue(job);

    m_waitCondition.wakeOne();
}

bool WorkerManager::dequeueJob(QString &job)
{
    QMutexLocker locker(&m_mutex);

    while (!m_aborted && m_queue.isEmpty()) {
        if (!m_waitCondition.wait(&m_mutex, WORKER_TIMEOUT_MSEC))
            break;  // timed out
    }

    if (m_aborted || m_queue.isEmpty())
        return false;

    job = m_queue.dequeue();
    return true;
}
