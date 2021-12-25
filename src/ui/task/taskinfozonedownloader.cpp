#include "taskinfozonedownloader.h"

#include <QDir>

#include <conf/confmanager.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <model/zonesourcewrapper.h>
#include <model/zonetypewrapper.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

#include "taskzonedownloader.h"

TaskInfoZoneDownloader::TaskInfoZoneDownloader(TaskManager &taskManager) :
    TaskInfo(ZoneDownloader, taskManager)
{
}

TaskZoneDownloader *TaskInfoZoneDownloader::zoneDownloader() const
{
    return static_cast<TaskZoneDownloader *>(taskWorker());
}

ZoneListModel *TaskInfoZoneDownloader::zoneListModel() const
{
    return IoC<ZoneListModel>();
}

bool TaskInfoZoneDownloader::processResult(bool success)
{
    if (!success)
        return false;

    IoC<WindowManager>()->showTrayMessage(
            tr("Zone Addresses Updated: %1.").arg(m_zoneNames.join(", ")),
            WindowManager::MessageZones);
    return true;
}

void TaskInfoZoneDownloader::loadZones()
{
    TaskZoneDownloader worker;

    const int rowCount = zoneListModel()->rowCount();
    for (m_zoneIndex = 0; m_zoneIndex < rowCount; ++m_zoneIndex) {
        setupTaskWorkerByZone(&worker);
        addSubResult(&worker, false);
    }

    emitZonesUpdated();
}

bool TaskInfoZoneDownloader::saveZoneAsText(const QString &filePath, int zoneIndex)
{
    TaskZoneDownloader worker;

    m_zoneIndex = zoneIndex;

    setupTaskWorkerByZone(&worker);

    return worker.saveAddressesAsText(filePath);
}

void TaskInfoZoneDownloader::setupTaskWorker()
{
    m_success = false;
    m_zoneIndex = 0;
    m_zonesMask = 0;
    m_zoneNames.clear();

    clearSubResults();

    setupNextTaskWorker();
}

void TaskInfoZoneDownloader::setupNextTaskWorker()
{
    const int rowCount = zoneListModel()->rowCount();
    if (m_zoneIndex >= rowCount) {
        emitZonesUpdated();

        TaskInfo::handleFinished(m_success);
        return;
    }

    abortTask();

    TaskInfo::setupTaskWorker();
    auto worker = zoneDownloader();

    setupTaskWorkerByZone(worker);
}

void TaskInfoZoneDownloader::setupTaskWorkerByZone(TaskZoneDownloader *worker)
{
    const auto zoneRow = zoneListModel()->zoneRowAt(m_zoneIndex);
    const auto zoneSource =
            ZoneSourceWrapper(zoneListModel()->zoneSourceByCode(zoneRow.sourceCode));
    const auto zoneType = ZoneTypeWrapper(zoneListModel()->zoneTypeByCode(zoneSource.zoneType()));

    worker->setZoneEnabled(zoneRow.enabled);
    worker->setSort(zoneType.sort());
    worker->setEmptyNetMask(zoneType.emptyNetMask());
    worker->setZoneId(zoneRow.zoneId);
    worker->setZoneName(zoneRow.zoneName);
    worker->setUrl(zoneRow.customUrl ? zoneRow.url : zoneSource.url());
    worker->setFormData(zoneRow.customUrl ? zoneRow.formData : zoneSource.formData());
    worker->setPattern(zoneType.pattern());
    worker->setAddressCount(zoneRow.addressCount);
    worker->setTextChecksum(zoneRow.textChecksum);
    worker->setBinChecksum(zoneRow.binChecksum);
    worker->setCachePath(cachePath());
    worker->setSourceModTime(zoneRow.sourceModTime);
    worker->setLastSuccess(zoneRow.lastSuccess);

    insertZoneId(m_zonesMask, zoneRow.zoneId);
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

    const auto zoneId = worker->zoneId();
    const auto addressCount = worker->addressCount();
    const auto textChecksum = worker->textChecksum();
    const auto binChecksum = worker->binChecksum();

    const auto sourceModTime = worker->sourceModTime();
    const auto now = QDateTime::currentDateTime();
    const auto lastSuccess = success ? now : worker->lastSuccess();

    IoC<ConfManager>()->updateZoneResult(
            zoneId, addressCount, textChecksum, binChecksum, sourceModTime, now, lastSuccess);

    addSubResult(worker, success);
}

void TaskInfoZoneDownloader::clearSubResults()
{
    m_dataZonesMask = 0;
    m_enabledMask = 0;
    m_dataSize = 0;
    m_zonesData.clear();
}

void TaskInfoZoneDownloader::addSubResult(TaskZoneDownloader *worker, bool success)
{
    if (success) {
        m_zoneNames.append(worker->zoneName());
    } else if (!worker->loadAddresses()) {
        return;
    }

    const auto zoneData = worker->zoneData();
    const int size = zoneData.size();

    if (size == 0)
        return;

    m_dataSize += size;
    m_zonesData.append(zoneData);

    insertZoneId(m_dataZonesMask, worker->zoneId());

    if (worker->zoneEnabled()) {
        insertZoneId(m_enabledMask, worker->zoneId());
    }
}

void TaskInfoZoneDownloader::emitZonesUpdated()
{
    emit zonesUpdated(m_dataZonesMask, m_enabledMask, m_dataSize, m_zonesData);

    removeOrphanCacheFiles();

    clearSubResults();
}

void TaskInfoZoneDownloader::insertZoneId(quint32 &zonesMask, int zoneId)
{
    zonesMask |= (quint32(1) << (zoneId - 1));
}

bool TaskInfoZoneDownloader::containsZoneId(quint32 zonesMask, int zoneId) const
{
    return (zonesMask & (quint32(1) << (zoneId - 1))) != 0;
}

void TaskInfoZoneDownloader::removeOrphanCacheFiles()
{
    const auto fileInfos = QDir(cachePath()).entryInfoList(QDir::Files);
    for (const auto &fi : fileInfos) {
        const auto zoneId = fi.baseName().toInt();
        if (zoneId != 0 && !containsZoneId(m_zonesMask, zoneId)) {
            FileUtil::removeFile(fi.filePath());
        }
    }
}

QString TaskInfoZoneDownloader::cachePath() const
{
    return IoC<FortSettings>()->cachePath() + "zones/";
}
