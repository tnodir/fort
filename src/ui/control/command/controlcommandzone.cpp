#include "controlcommandzone.h"

#include <conf/confzonemanager.h>
#include <conf/zone.h>
#include <fortglobal.h>
#include <model/zonelistmodel.h>
#include <model/zonesourcewrapper.h>
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <util/conf/confutil.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum ZoneAction : qint8 {
    ZoneActionInvalid = -1,
    ZoneActionUpdate = 0,
    ZoneActionAdd,
    ZoneActionDel,
};

QStringList zoneActionNames()
{
    // Sync with enum ZoneAction
    return { "update", "add", "del" };
}

ZoneAction zoneActionByText(const QString &commandText)
{
    return ZoneAction(zoneActionNames().indexOf(commandText));
}

int zoneActionArgsCount(ZoneAction zoneAction)
{
    // The add action has the <file-path> argument
    switch (zoneAction) {
    case ZoneActionAdd:
        return 3;
    case ZoneActionDel:
        return 2;
    default:
        return 1;
    }
}

// The action with its arguments, else invalid
ZoneAction validZoneAction(const QVariantList &args)
{
    const ZoneAction zoneAction = zoneActionByText(args.value(0).toString());

    return (args.size() >= zoneActionArgsCount(zoneAction)) ? zoneAction : ZoneActionInvalid;
}

bool fillZoneSource(ProcessCommandResult &r, Zone &zone, const QString &sourceCode)
{
    const ZoneSourceWrapper zoneSource(zoneListModel()->zoneSourceByCode(sourceCode));

    if (zoneSource.code() != sourceCode) {
        r.errorMessage = ControlCommandUtil::invalidValueText("source", sourceCode);
        return false;
    }

    zone.sourceCode = sourceCode;

    return true;
}

bool fillZoneFields(ProcessCommandResult &r, Zone &zone, const QVariantList &args, int index)
{
    QVariantMap fields = {
        { "source", zone.sourceCode },
        { "enabled", zone.enabled },
        { "customUrl", zone.customUrl },
        { "url", zone.url },
        { "formData", zone.formData },
    };

    if (!ControlCommandUtil::parseFieldArgs(args, index, fields, r.errorMessage))
        return false;

    if (!fillZoneSource(r, zone, fields["source"].toString()))
        return false;

    zone.enabled = fields["enabled"].toBool();
    zone.customUrl = fields["customUrl"].toBool();
    zone.url = fields["url"].toString();
    zone.formData = fields["formData"].toString();

    return true;
}

bool addZone(ProcessCommandResult &r, const QVariantList &args)
{
    Zone zone;
    zone.zoneName = args.value(1).toString();
    zone.sourceCode = "text";
    zone.textInline = args.value(2).toString();

    if (zone.zoneName.isEmpty()) {
        r.errorMessage = "<zone-name> is empty";
        return false;
    }

    if (!fillZoneFields(r, zone, args, 3))
        return false;

    if (!confZoneManager()->addOrUpdateZone(zone))
        return false;

    r.errorMessage = QString::number(zone.zoneId);

    return true;
}

quint8 argZoneId(const QVariantList &args)
{
    const uint zoneId = args.value(1).toUInt();

    return (zoneId <= uint(ConfUtil::zoneMaxCount())) ? zoneId : 0;
}

bool deleteZone(ProcessCommandResult &r, const QVariantList &args)
{
    auto confZoneManager = Fort::confZoneManager();

    const quint8 zoneId = argZoneId(args);

    if (zoneId == 0 || confZoneManager->zoneNameById(zoneId).isEmpty()) {
        ControlCommandUtil::setErrorResult(r, "Zone not found");
        return true;
    }

    return confZoneManager->deleteZone(zoneId);
}

bool processCommandZoneAction(
        ProcessCommandResult &r, const QVariantList &args, ZoneAction zoneAction)
{
    switch (zoneAction) {
    case ZoneActionUpdate: {
        taskManager()->runTask(TaskInfo::ZoneDownloader);
        return true;
    }
    case ZoneActionAdd: {
        return addZone(r, args);
    }
    default: // Del
        return deleteZone(r, args);
    }
}

}

bool ControlCommandZone::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    const ZoneAction zoneAction = validZoneAction(p.args);

    if (zoneAction == ZoneActionInvalid) {
        r.errorMessage = "Usage: zone update"
                         " | zone add <zone-name> <file-path> [<key>=<value>...]"
                         " | zone del <zone-id>";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, zoneAction))
        return false;

    return processCommandZoneAction(r, p.args, zoneAction);
}

ControlCommandBase::ClientFileArg ControlCommandZone::clientFileArg(const QVariantList &args) const
{
    return (validZoneAction(args) == ZoneActionAdd) ? ClientFileRead : ClientFileNone;
}
