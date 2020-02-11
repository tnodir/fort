#include "taskinfozonedownloader.h"

#include <QDir>

#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../model/zonelistmodel.h"
#include "../model/zonesourcewrapper.h"
#include "../model/zonetypewrapper.h"
#include "../util/fileutil.h"
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

    removeOrphanCacheFiles();

    fortManager()->showTrayMessage(tr("Zone Addresses Updated!"));
    return true;
}

void TaskInfoZoneDownloader::setupTaskWorker()
{
    const int rowCount = zoneListModel()->rowCount();
    if (m_zoneIndex >= rowCount) {
        m_zoneIndex = 0;

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

    worker->setStoreText(zoneRow.storeText);
    worker->setSort(zoneType.sort());
    worker->setEmptyNetMask(zoneType.emptyNetMask());
    worker->setZoneId(zoneRow.zoneId);
    worker->setUrl(zoneRow.customUrl ? zoneRow.url
                                     : zoneSource.url());
    worker->setFormData(zoneRow.customUrl ? zoneRow.formData
                                          : zoneSource.formData());
    worker->setPattern(zoneType.pattern());
    worker->setTextChecksum(zoneRow.textChecksum);
    worker->setCachePath(cachePath());
    worker->setSourceModTime(zoneRow.sourceModTime);
    worker->setLastSuccess(zoneRow.lastSuccess);

    m_zoneIdSet.insert(zoneRow.zoneId);
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

    setupTaskWorker();
    runTaskWorker();
}

void TaskInfoZoneDownloader::processSubResult(bool success)
{
    if (aborted() && !success)
        return;

    auto worker = zoneDownloader();

    const auto zoneId = worker->zoneId();
    const auto textChecksum = worker->textChecksum();
    const auto binChecksum = worker->binChecksum();

    const auto sourceModTime = worker->sourceModTime();
    const auto now = QDateTime::currentDateTime();
    const auto lastSuccess = success ? now : worker->lastSuccess();

    zoneListModel()->updateZoneResult(zoneId, textChecksum, binChecksum,
                                      sourceModTime, now, lastSuccess);
}

void TaskInfoZoneDownloader::removeOrphanCacheFiles()
{
    for (const auto fi : QDir(cachePath()).entryInfoList(QDir::Files)) {
        const auto zoneId = fi.baseName().toInt();
        if (zoneId != 0 && !m_zoneIdSet.contains(zoneId)) {
            FileUtil::removeFile(fi.filePath());
        }
    }
}

QString TaskInfoZoneDownloader::cachePath() const
{
    return fortManager()->settings()->cachePath() + "zones/";
}
