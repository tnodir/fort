#ifndef HOSTINFOJOB_H
#define HOSTINFOJOB_H

#include <util/worker/workerjob.h>

class HostInfoManager;

class HostInfoJob : public WorkerJob
{
public:
    explicit HostInfoJob(const QString &address);

    QString address() const { return text(); }

    void doJob(WorkerObject &worker) override;
    void reportResult(WorkerObject &worker) override;

private:
    void emitFinished(HostInfoManager *manager);

private:
    QString m_hostName;
};

#endif // HOSTINFOJOB_H
