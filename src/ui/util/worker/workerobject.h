#ifndef WORKEROBJECT_H
#define WORKEROBJECT_H

#include <QObject>
#include <QRunnable>

QT_FORWARD_DECLARE_CLASS(WorkerManager)

class WorkerObject : public QRunnable
{
public:
    explicit WorkerObject(WorkerManager *manager);

    WorkerManager *manager() const { return m_manager; }

    bool aborted() const { return m_aborted; }
    virtual void abort() { m_aborted = true; }

    void run() override;

protected:
    virtual void doJob(const QString &job) = 0;

private:
    volatile bool m_aborted;

    WorkerManager *m_manager;
};

#endif // WORKEROBJECT_H
