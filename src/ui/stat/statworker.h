#ifndef STATWORKER_H
#define STATWORKER_H

#include <util/worker/workerobject.h>

class StatManager;

class StatWorker : public WorkerObject
{
public:
    explicit StatWorker(StatManager *manager);

    QThread::Priority priority() const override { return QThread::LowPriority; }
};

#endif // STATWORKER_H
