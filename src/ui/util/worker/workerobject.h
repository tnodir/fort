#ifndef WORKEROBJECT_H
#define WORKEROBJECT_H

#include <QObject>
#include <QRunnable>
#include <QThread>

class WorkerJob;
class WorkerManager;

class WorkerObject : public QRunnable
{
public:
    explicit WorkerObject(WorkerManager *manager);

    WorkerManager *manager() const { return m_manager; }

    virtual QThread::Priority priority() const { return QThread::NormalPriority; }

    void run() override;

protected:
    virtual void doJob(WorkerJob *job);

private:
    WorkerManager *m_manager = nullptr;
};

#endif // WORKEROBJECT_H
