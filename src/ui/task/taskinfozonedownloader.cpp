#include "taskinfozonedownloader.h"

#include "../conf/addressgroup.h"
#include "../conf/firewallconf.h"
#include "../fortmanager.h"
#include "taskzonedownloader.h"

TaskInfoZoneDownloader::TaskInfoZoneDownloader(QObject *parent) :
    TaskInfo(Tasix, parent)
{
}

bool TaskInfoZoneDownloader::processResult(FortManager *fortManager, bool success)
{
    if (!success)
        return false;

    const auto worker = static_cast<TaskZoneDownloader *>(taskWorker());

    FirewallConf *conf = fortManager->conf();
    AddressGroup *inetGroup = conf->inetAddressGroup();

    if (inetGroup->excludeText() == worker->rangeText())
        return false;

    inetGroup->setExcludeText(worker->rangeText());

    return fortManager->saveOriginConf(tr("TAS-IX addresses updated!"));
}
