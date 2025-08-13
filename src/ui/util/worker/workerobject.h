#ifndef WORKEROBJECT_H
#define WORKEROBJECT_H

#include <QObject>
#include <QRunnable>
#include <QThread>

#include "worker_types.h"

class WorkerObject : public QRunnable
{
public:
    explicit WorkerObject(WorkerManager *manager);

    WorkerManager *manager() const { return m_manager; }

    virtual QThread::Priority priority() const { return QThread::NormalPriority; }

    virtual QString workerName() const;

    void run() override;

protected:
    void doJobs();

    virtual void doJob(WorkerJob &job);

private:
    WorkerManager *m_manager = nullptr;
};

#endif // WORKEROBJECT_H
