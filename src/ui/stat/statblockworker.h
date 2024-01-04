#ifndef STATBLOCKWORKER_H
#define STATBLOCKWORKER_H

#include <util/worker/workerobject.h>

class StatBlockManager;

class StatBlockWorker : public WorkerObject
{
public:
    explicit StatBlockWorker(StatBlockManager *manager);

    QThread::Priority priority() const override { return QThread::HighPriority; }

    QString workerName() const override { return "StatBlockWorker"; }
};

#endif // STATBLOCKWORKER_H
