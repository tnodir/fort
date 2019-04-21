#ifndef WORKEROBJECT_H
#define WORKEROBJECT_H

#include <QObject>
#include <QRunnable>

QT_FORWARD_DECLARE_CLASS(WorkerJob)
QT_FORWARD_DECLARE_CLASS(WorkerManager)

class WorkerObject : public QRunnable
{
public:
    explicit WorkerObject(WorkerManager *manager);

    WorkerManager *manager() const { return m_manager; }

    void run() override;

protected:
    virtual void doJob(WorkerJob *job);

private:
    WorkerManager *m_manager;
};

#endif // WORKEROBJECT_H
