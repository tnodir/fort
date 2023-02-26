#ifndef WORKERJOB_H
#define WORKERJOB_H

#include <QObject>

#include <util/classhelpers.h>

class WorkerObject;

class WorkerJob
{
public:
    explicit WorkerJob(const QString &text = {});
    virtual ~WorkerJob() = default;

    const QString &text() const { return m_text; }

    virtual void doJob(WorkerObject *worker) { Q_UNUSED(worker); }
    virtual void reportResult(WorkerObject *worker) { Q_UNUSED(worker); }

public:
    const QString m_text;
};

#endif // WORKERJOB_H
