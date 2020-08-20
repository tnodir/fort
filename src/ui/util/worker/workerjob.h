#ifndef WORKERJOB_H
#define WORKERJOB_H

#include <QObject>

#include "../classhelpers.h"

class WorkerJob
{
public:
    explicit WorkerJob(const QString &_text);
    virtual ~WorkerJob() = default;
    CLASS_DEFAULT_COPY_MOVE(WorkerJob)

    virtual void doJob() { }

public:
    QString text;
};

#endif // WORKERJOB_H
