#include "hostinfomanager.h"

#include "hostinfoworker.h"

HostInfoManager::HostInfoManager(QObject *parent) :
    WorkerManager(parent)
{
    setMaxWorkersCount(2);

    QSysInfo::machineHostName();  // Initialize ws2_32.dll
}

WorkerObject *HostInfoManager::createWorker()
{
    return new HostInfoWorker(this);
}

void HostInfoManager::lookupHost(const QString &address)
{
    enqueueJob(address);
}

void HostInfoManager::handleWorkerResult(const QString &address,
                                         const QVariant &hostName)
{
    emit lookupFinished(address, hostName.toString());
}
