#ifndef WORKERJOB_H
#define WORKERJOB_H

#include <QObject>

#include <util/classhelpers.h>

#include "worker_types.h"

class WorkerJob
{
public:
    explicit WorkerJob(const QString &text = {});
    virtual ~WorkerJob() = default;

    const QString &text() const { return m_text; }

    virtual bool mergeJob(const WorkerJob &job)
    {
        Q_UNUSED(job);
        return false;
    }

    virtual void doJob(WorkerManager * /*manager*/) { }
    virtual void reportResult(WorkerManager * /*manager*/) { }

private:
    const QString m_text;
};

#endif // WORKERJOB_H
