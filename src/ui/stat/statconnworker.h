#ifndef STATCONNWORKER_H
#define STATCONNWORKER_H

#include <util/worker/workerobject.h>

class StatConnManager;

class StatConnWorker : public WorkerObject
{
public:
    explicit StatConnWorker(StatConnManager *manager);

    QThread::Priority priority() const override { return QThread::HighPriority; }

    QString workerName() const override { return "StatConnWorker"; }
};

#endif // STATCONNWORKER_H
