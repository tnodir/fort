#ifndef WORKERJOB_H
#define WORKERJOB_H

#include <QObject>

#include <util/classhelpers.h>

#include "workertypes.h"

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

    virtual void doJob(WorkerObject &worker) { Q_UNUSED(worker); }
    virtual void reportResult(WorkerObject &worker) { Q_UNUSED(worker); }

private:
    const QString m_text;
};

#endif // WORKERJOB_H
