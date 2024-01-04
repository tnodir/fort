#ifndef WORKERMANAGER_H
#define WORKERMANAGER_H

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QRunnable>
#include <QVariant>
#include <QWaitCondition>

#include <util/classhelpers.h>

#include "workertypes.h"

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

    virtual QString workerName() const { return QString(); }

public slots:
    void clear();
    void abortWorkers();

    void enqueueJob(WorkerJobPtr job);
    WorkerJobPtr dequeueJob();

    void workerFinished(WorkerObject *worker);

protected:
    virtual WorkerObject *createWorker();
    virtual bool canMergeJobs() const { return false; }

    int jobCount() const;

    bool mergeJob(WorkerJobPtr job);

private:
    void setupWorker();

    bool checkNewWorkerNeeded() const;

    void clearJobQueue();

private:
    volatile bool m_aborted = false;

    int m_maxWorkersCount = 0;

    QList<WorkerObject *> m_workers;

    QQueue<WorkerJobPtr> m_jobQueue;

    mutable QMutex m_mutex;
    QWaitCondition m_jobWaitCondition;
    QWaitCondition m_abortWaitCondition;
};

#endif // WORKERMANAGER_H
