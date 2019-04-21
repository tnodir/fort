#ifndef WORKERJOB_H
#define WORKERJOB_H

#include <QObject>

class WorkerJob
{
public:
    explicit WorkerJob(const QString &_text);
    virtual ~WorkerJob() {}

    virtual void doJob() {}

public:
    QString text;
};

#endif // WORKERJOB_H
