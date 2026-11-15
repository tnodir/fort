#include "hostzonessyncjob.h"

#include <util/net/hostrange.h>
#include <util/worker/workermanager.h>

#include "hostzonesdb.h"
#include "hostzonessync.h"

HostZonesSyncJob::HostZonesSyncJob(const QString &filePath, const HostZonesSource &source) :
    WorkerJob(filePath), m_source(source)
{
}

void HostZonesSyncJob::doJob(WorkerManager *manager)
{
    HostZonesDb db(filePath());

    if (db.open()) {
        syncZones(manager, db);
    }

    m_errorMessage = db.errorMessage();
}

void HostZonesSyncJob::reportResult(WorkerManager *manager)
{
    emitSynced(static_cast<HostZonesSync *>(manager));
}

void HostZonesSyncJob::syncZones(WorkerManager *manager, HostZonesDb &db)
{
    HostRange hostRange;

    for (const HostZoneSource &zone : std::as_const(m_source.zones)) {
        // The forwarder's stop waits for the Zone's transaction only
        if (manager->aborted())
            return;

        if (TaskInfoZoneDownloader::syncHostZone(db, zone, hostRange)) {
            m_changed = true;
        }
    }

    if (db.retainZones(m_source.hostsMask)) {
        m_changed = true;
    }
}

void HostZonesSyncJob::emitSynced(HostZonesSync *manager)
{
    emit manager->synced(m_changed, m_errorMessage);
}
