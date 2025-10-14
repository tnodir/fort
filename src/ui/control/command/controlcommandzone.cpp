#include "controlcommandzone.h"

#include <fortglobal.h>
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>

using namespace Fort;

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
        taskManager()->runTask(TaskInfo::ZoneDownloader);
        return true;
    }
    default:
        return false;
    }
}

}

bool ControlCommandZone::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const ZoneAction zoneAction = zoneActionByText(p.args.value(0).toString());
    if (zoneAction == ZoneActionNone) {
        r.errorMessage = "Usage: zone update";
        return false;
    }

    if (!checkCommandActionPassword(r, zoneAction))
        return false;

    const bool ok = processCommandZoneAction(zoneAction);

    uncheckCommandActionPassword();

    return ok;
}
