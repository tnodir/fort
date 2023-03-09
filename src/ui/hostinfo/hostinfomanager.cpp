#include "hostinfomanager.h"

#include "hostinfojob.h"

HostInfoManager::HostInfoManager(QObject *parent) : WorkerManager(parent)
{
    setMaxWorkersCount(2);

    QSysInfo::machineHostName(); // Initialize ws2_32.dll
}

void HostInfoManager::lookupHost(const QString &address)
{
    enqueueJob(WorkerJobPtr(new HostInfoJob(address)));
}
