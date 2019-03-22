#ifndef HOSTINFOWORKER_H
#define HOSTINFOWORKER_H

#include "../worker/workerobject.h"

QT_FORWARD_DECLARE_CLASS(HostInfoManager)

class HostInfoWorker : public WorkerObject
{
public:
    explicit HostInfoWorker(HostInfoManager *manager);

protected:
    void doJob(const QString &address) override;
};

#endif // HOSTINFOWORKER_H
