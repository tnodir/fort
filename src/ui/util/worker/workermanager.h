#ifndef WORKERMANAGER_H
#define WORKERMANAGER_H

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QRunnable>
#include <QVariant>
#include <QWaitCondition>

#include "../classhelpers.h"

class WorkerJob;
class WorkerObject;

class WorkerManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkerManager(QObject *parent = nullptr);
    ~WorkerManager() override;
    CLASS_DELETE_COPY_MOVE(WorkerManager)

    bool aborted() const { return m_aborted; }

    int maxWorkersCount() const { return m_maxWorkersCount; }
    void setMaxWorkersCount(int v) { m_maxWorkersCount = v; }

signals:

public slots:
    void clear();
    void abort();

    void enqueueJob(WorkerJob *job);
    WorkerJob *dequeueJob();

    void workerFinished(WorkerObject *worker);

    virtual void handleWorkerResult(WorkerJob *job) = 0;

protected:
    virtual WorkerObject *createWorker();

private:
    void setupWorker();

private:
    volatile bool m_aborted = false;

    int m_maxWorkersCount = 0;

    QList<WorkerObject *> m_workers;

    QQueue<WorkerJob *> m_jobQueue;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

#endif // WORKERMANAGER_H
