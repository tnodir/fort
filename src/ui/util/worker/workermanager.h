#ifndef WORKERMANAGER_H
#define WORKERMANAGER_H

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QRunnable>
#include <QVariant>
#include <QWaitCondition>

QT_FORWARD_DECLARE_CLASS(WorkerObject)

class WorkerManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkerManager(QObject *parent = nullptr);
    ~WorkerManager() override;

    int maxWorkersCount() const { return m_maxWorkersCount; }
    void setMaxWorkersCount(int v) { m_maxWorkersCount = v; }

signals:

public slots:
    void clear();

    void enqueueJob(const QString &job);
    bool dequeueJob(QString &job);

    void workerFinished(WorkerObject *worker);

    virtual void handleWorkerResult(const QString &job,
                                    const QVariant &result) = 0;

protected:
    virtual WorkerObject *createWorker() = 0;

private:
    void setupWorker();

    void abort();
    void abortWorkers();

private:
    volatile bool m_aborted;

    int m_maxWorkersCount;

    QList<WorkerObject *> m_workers;

    QQueue<QString> m_queue;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

#endif // WORKERMANAGER_H
