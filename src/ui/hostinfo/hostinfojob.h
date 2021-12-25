#ifndef HOSTINFOJOB_H
#define HOSTINFOJOB_H

#include <util/worker/workerjob.h>

class HostInfoJob : public WorkerJob
{
public:
    explicit HostInfoJob(const QString &address);

    QString address() const { return text; }

    void doJob() override;

public:
    QString hostName;
};

#endif // HOSTINFOJOB_H
