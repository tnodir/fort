#include "taskinfozonedownloader.h"

#include <QDir>
#include <QLoggingCategory>

#include <conf/confzonemanager.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <model/zonelistmodel.h>
#include <model/zonesourcewrapper.h>
#include <model/zonetypewrapper.h>
#include <util/dateutil.h>
#include <util/fileutil.h>

#include "taskmanager.h"
#include "taskzonedownloader.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("task.zoneDownloader");

}

TaskInfoZoneDownloader::TaskInfoZoneDownloader(TaskManager &taskManager) :
    TaskInfo(ZoneDownloader, taskManager)
{
}

TaskZoneDownloader *TaskInfoZoneDownloader::zoneDownloader() const
{
    return static_cast<TaskZoneDownloader *>(taskWorker());
}

void TaskInfoZoneDownloader::initialize()
{
    loadZones();
}

bool TaskInfoZoneDownloader::processResult(bool success)
{
    if (!success) {
        qCDebug(LC) << "No updates";
        return false;
    }

    qCDebug(LC) << "Updated:" << m_zoneNames;

    emit taskManager()->zonesDownloaded(m_zoneNames);

    return true;
}

ZonesData TaskInfoZoneDownloader::loadZonesData() const
{
    ZonesData zones;

    TaskZoneDownloader worker;

    const int rowCount = zoneListModel()->rowCount();
    for (int zoneIndex = 0; zoneIndex < rowCount; ++zoneIndex) {
        setupTaskWorkerByZone(&worker, zoneIndex);

        insertZoneId(zones.zonesMask, worker.zoneId());

        if (worker.loadAddresses()) {
            addZoneData(zones, worker);
        }
    }

    return zones;
}

bool TaskInfoZoneDownloader::saveZoneAsText(const QString &filePath, int zoneIndex) const
{
    TaskZoneDownloader worker;

    setupTaskWorkerByZone(&worker, zoneIndex);

    return worker.saveAddressesAsText(filePath);
}

void TaskInfoZoneDownloader::setupTaskWorker()
{
    m_success = false;
    m_zoneIndex = 0;
    m_zoneNames.clear();

    clearSubResults();

    setupNextTaskWorker();
}

void TaskInfoZoneDownloader::setupNextTaskWorker()
{
    const int rowCount = zoneListModel()->rowCount();
    if (m_zoneIndex >= rowCount) {
        if (aborted()) {
            loadZones(); // the rest of the Zones from the cache
        }

        emitZonesUpdated();

        TaskInfo::handleFinished(m_success);
        return;
    }

    abortTask();

    TaskInfo::setupTaskWorker();
    auto worker = zoneDownloader();

    setupTaskWorkerByZone(worker, m_zoneIndex);

    insertZoneId(m_zones.zonesMask, worker->zoneId());
}

void TaskInfoZoneDownloader::setupTaskWorkerByZone(TaskZoneDownloader *worker, int zoneIndex) const
{
    const auto &zoneRow = zoneListModel()->zoneRowAt(zoneIndex);

    const ZoneSourceWrapper zoneSource(zoneListModel()->zoneSourceByCode(zoneRow.sourceCode));
    const ZoneTypeWrapper zoneType(zoneListModel()->zoneTypeByCode(zoneSource.zoneType()));

    worker->setZoneEnabled(zoneRow.enabled);
    worker->setSort(zoneType.sort());
    worker->setEmptyNetMask(zoneType.emptyNetMask());
    worker->setZoneId(zoneRow.zoneId);
    worker->setZoneName(zoneRow.zoneName);
    worker->setUrl(zoneRow.customUrl ? zoneRow.url : zoneSource.url());
    worker->setFormData(zoneRow.customUrl ? zoneRow.formData : zoneSource.formData());
    worker->setTextInline(zoneRow.textInline);
    worker->setPattern(zoneType.pattern());
    worker->setAddressCount(zoneRow.addressCount);
    worker->setTextChecksum(zoneRow.textChecksum);
    worker->setBinChecksum(zoneRow.binChecksum);
    worker->setCachePath(cachePath());
    worker->setSourceModTime(zoneRow.sourceModTime);
    worker->setLastSuccess(zoneRow.lastSuccess);
}

void TaskInfoZoneDownloader::handleFinished(bool success)
{
    processSubResult(success);

    if (success) {
        m_success = true;
    }

    if (aborted()) {
        m_zoneIndex = INT_MAX;
    } else {
        ++m_zoneIndex;
    }

    setupNextTaskWorker();
    runTaskWorker();
}

void TaskInfoZoneDownloader::processSubResult(bool success)
{
    if (aborted() && !success)
        return;

    auto worker = zoneDownloader();

    Zone zone;
    zone.zoneId = worker->zoneId();
    zone.addressCount = worker->addressCount();
    zone.textChecksum = worker->textChecksum();
    zone.binChecksum = worker->binChecksum();

    zone.sourceModTime = worker->sourceModTime();
    zone.lastRun = DateUtil::now();
    zone.lastSuccess = success ? zone.lastRun : worker->lastSuccess();

    confZoneManager()->updateZoneResult(zone);

    addSubResult(worker, success);
}

void TaskInfoZoneDownloader::clearSubResults()
{
    m_zones = {};
}

void TaskInfoZoneDownloader::addSubResult(TaskZoneDownloader *worker, bool success)
{
    if (success) {
        m_zoneNames.append(worker->zoneName());
    } else if (!worker->loadAddresses()) {
        return;
    }

    addZoneData(m_zones, *worker);
}

void TaskInfoZoneDownloader::addZoneData(ZonesData &zones, const TaskZoneDownloader &worker)
{
    const auto &zoneData = worker.zoneData();
    const int size = zoneData.size();

    if (size == 0)
        return;

    zones.dataSize += size;
    zones.zonesData.append(zoneData);

    insertZoneId(zones.dataZonesMask, worker.zoneId());

    if (worker.zoneEnabled()) {
        insertZoneId(zones.enabledMask, worker.zoneId());
    }
}

void TaskInfoZoneDownloader::emitZonesUpdated()
{
    emit taskManager()->zonesUpdated(
            m_zones.dataZonesMask, m_zones.enabledMask, m_zones.dataSize, m_zones.zonesData);

    removeOrphanCacheFiles();

    clearSubResults();
}

void TaskInfoZoneDownloader::insertZoneId(quint32 &zonesMask, int zoneId)
{
    zonesMask |= (quint32(1) << (zoneId - 1));
}

bool TaskInfoZoneDownloader::containsZoneId(quint32 zonesMask, int zoneId)
{
    return (zonesMask & (quint32(1) << (zoneId - 1))) != 0;
}

void TaskInfoZoneDownloader::loadZones()
{
    m_zones = loadZonesData();
}

void TaskInfoZoneDownloader::removeOrphanCacheFiles()
{
    const auto fileInfos = QDir(cachePath()).entryInfoList(QDir::Files);
    for (const auto &fi : fileInfos) {
        const auto zoneId = fi.baseName().toInt();
        if (zoneId != 0 && !containsZoneId(m_zones.zonesMask, zoneId)) {
            FileUtil::removeFile(fi.filePath());
        }
    }
}

QString TaskInfoZoneDownloader::cachePath() const
{
    return settings()->cachePath() + "zones/";
}
