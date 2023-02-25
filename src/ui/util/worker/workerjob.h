#ifndef WORKERJOB_H
#define WORKERJOB_H

#include <QObject>

#include <util/classhelpers.h>

class WorkerObject;

class WorkerJob
{
public:
    explicit WorkerJob(const QString &_text);
    virtual ~WorkerJob() = default;
    CLASS_DEFAULT_COPY_MOVE(WorkerJob)

    virtual void doJob(WorkerObject *worker) { }
    virtual void reportResult(WorkerObject *worker) { }

public:
    QString text;
};

#endif // WORKERJOB_H
