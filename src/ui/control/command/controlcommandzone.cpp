#include "controlcommandzone.h"

#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

enum ZoneAction : quint32 {
    ZoneActionNone = 0,
    ZoneActionUpdate = (1 << 0),
};

ZoneAction zoneActionByText(const QString &commandText)
{
    if (commandText == "update")
        return ZoneActionUpdate;

    return ZoneActionNone;
}

bool processCommandZoneAction(ZoneAction zoneAction)
{
    switch (zoneAction) {
    case ZoneActionUpdate: {
        IoC<TaskManager>()->runTask(TaskInfo::ZoneDownloader);
        return true;
    }
    default:
        return false;
    }
}

}

bool ControlCommandZone::processCommand(const ProcessCommandArgs &p)
{
    const ZoneAction zoneAction = zoneActionByText(p.args.value(0).toString());
    if (zoneAction == ZoneActionNone) {
        p.errorMessage = "Usage: zone update";
        return false;
    }

    if (!checkCommandActionPassword(p, zoneAction))
        return false;

    return processCommandZoneAction(zoneAction);
}
