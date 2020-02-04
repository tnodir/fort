#include "taskinfozonedownloader.h"

#include "../conf/addressgroup.h"
#include "../conf/firewallconf.h"
#include "../fortmanager.h"
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

    const auto worker = zoneDownloader();

    FirewallConf *conf = fortManager()->conf();
    AddressGroup *inetGroup = conf->inetAddressGroup();

    if (inetGroup->excludeText() == worker->rangeText())
        return false;

    inetGroup->setExcludeText(worker->rangeText());

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
    worker->setUrl(zoneRow.customUrl ? zoneRow.url
                                     : zoneSource.url());
    worker->setFormData(zoneRow.customUrl ? zoneRow.formData
                                          : zoneSource.formData());
    worker->setPattern(zoneType.pattern());
}

void TaskInfoZoneDownloader::handleFinished(bool success)
{
    if (success) {
        m_success = true;
    }

    ++m_zoneIndex;

    setupTaskWorker();
    runTaskWorker();
}
