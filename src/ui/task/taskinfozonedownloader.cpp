#include "taskinfozonedownloader.h"

#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../model/zonelistmodel.h"
#include "../model/zonesourcewrapper.h"
#include "../model/zonetypewrapper.h"
#include "taskzonedownloader.h"

TaskInfoZoneDownloader::TaskInfoZoneDownloader(FortManager *fortManager,
                                               QObject *parent) :
    TaskInfo(ZoneDownloader, fortManager, parent)
{
}

TaskZoneDownloader *TaskInfoZoneDownloader::zoneDownloader() const
{
    return static_cast<TaskZoneDownloader *>(taskWorker());
}

ZoneListModel *TaskInfoZoneDownloader::zoneListModel() const
{
    return fortManager()->zoneListModel();
}

bool TaskInfoZoneDownloader::processResult(bool success)
{
    if (!success)
        return false;

    return fortManager()->saveOriginConf(tr("Zone Addresses Updated!"));
}

void TaskInfoZoneDownloader::setupTaskWorker()
{
    const int rowCount = zoneListModel()->rowCount();
    if (m_zoneIndex >= rowCount || aborted()) {
        TaskInfo::handleFinished(m_success);
        return;
    }

    abort();

    TaskInfo::setupTaskWorker();
    auto worker = zoneDownloader();

    const auto zoneRow = zoneListModel()->zoneRowAt(m_zoneIndex);
    const auto zoneSource = ZoneSourceWrapper(
                zoneListModel()->zoneSourceByCode(zoneRow.sourceCode));
    const auto zoneType = ZoneTypeWrapper(
                zoneListModel()->zoneTypeByCode(zoneSource.zoneType()));

    worker->setSort(zoneType.sort());
    worker->setEmptyNetMask(zoneType.emptyNetMask());
    worker->setZoneId(zoneRow.zoneId);
    worker->setUrl(zoneRow.customUrl ? zoneRow.url
                                     : zoneSource.url());
    worker->setFormData(zoneRow.customUrl ? zoneRow.formData
                                          : zoneSource.formData());
    worker->setPattern(zoneType.pattern());
    worker->setChecksum(zoneRow.checksum);
    worker->setCachePath(cachePath());
    worker->setLastSuccess(zoneRow.lastSuccess);
}

void TaskInfoZoneDownloader::handleFinished(bool success)
{
    processSubResult(success);

    if (success) {
        m_success = true;
    }

    ++m_zoneIndex;

    setupTaskWorker();
    runTaskWorker();
}

void TaskInfoZoneDownloader::processSubResult(bool success)
{
    if (aborted() && !success)
        return;

    auto worker = zoneDownloader();

    const auto zoneId = worker->zoneId();
    const auto checksum = worker->checksum();

    const auto now = QDateTime::currentDateTime();
    const auto lastSuccess = success ? now : worker->lastSuccess();

    zoneListModel()->updateZoneResult(zoneId, checksum, now, lastSuccess);
}

QString TaskInfoZoneDownloader::cachePath() const
{
    return fortManager()->settings()->cachePath() + "zones/";
}
