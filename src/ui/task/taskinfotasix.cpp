#include "taskinfotasix.h"

#include "../conf/addressgroup.h"
#include "../conf/firewallconf.h"
#include "../fortmanager.h"
#include "tasktasix.h"

TaskInfoTasix::TaskInfoTasix(QObject *parent) :
    TaskInfo(Tasix, parent)
{
}

bool TaskInfoTasix::processResult(FortManager *fortManager, bool success)
{
    if (!success)
        return false;

    const auto tasix = static_cast<TaskTasix *>(taskWorker());

    FirewallConf *conf = fortManager->conf();
    AddressGroup *inetGroup = conf->inetAddressGroup();

    if (inetGroup->excludeText() == tasix->rangeText())
        return false;

    inetGroup->setExcludeText(tasix->rangeText());

    return fortManager->saveOriginConf(tr("TAS-IX addresses updated!"));
}
