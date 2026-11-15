#include "hostzonessync.h"

#include "hostzonessyncjob.h"

HostZonesSync::HostZonesSync(QObject *parent) : WorkerManager(parent)
{
    setMaxWorkersCount(1);
}

void HostZonesSync::sync(const QString &filePath, const HostZonesSource &source)
{
    enqueueJob(WorkerJobPtr(new HostZonesSyncJob(filePath, source)));
}
