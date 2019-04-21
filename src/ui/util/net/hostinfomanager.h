#ifndef HOSTINFOMANAGER_H
#define HOSTINFOMANAGER_H

#include "../worker/workermanager.h"

class HostInfoManager : public WorkerManager
{
    Q_OBJECT

public:
    explicit HostInfoManager(QObject *parent = nullptr);

signals:
    void lookupFinished(const QString &address, const QString &hostName);

public slots:
    void lookupHost(const QString &address);

    void handleWorkerResult(WorkerJob *workerJob) override;
};

#endif // HOSTINFOMANAGER_H
