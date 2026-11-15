#ifndef HOSTZONESSYNCJOB_H
#define HOSTZONESSYNCJOB_H

#include <task/taskinfozonedownloader.h>
#include <util/worker/workerjob.h>

class HostZonesSync;

class HostZonesSyncJob : public WorkerJob
{
public:
    explicit HostZonesSyncJob(const QString &filePath, const HostZonesSource &source);

    QString filePath() const { return text(); }

    void doJob(WorkerManager *manager) override;
    void reportResult(WorkerManager *manager) override;

private:
    void emitSynced(HostZonesSync *manager);

private:
    bool m_changed = false;

    HostZonesSource m_source;
};

#endif // HOSTZONESSYNCJOB_H
