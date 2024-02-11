#include "zonescontroller.h"

#include <conf/confzonemanager.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <util/ioc/ioccontainer.h>

namespace {

void showErrorMessage(const QString &errorMessage)
{
    IoC<WindowManager>()->showErrorBox(
            errorMessage, ZonesController::tr("Zone Configuration Error"));
}

}

ZonesController::ZonesController(QObject *parent) : BaseController(parent) { }

ZoneListModel *ZonesController::zoneListModel() const
{
    return IoC<ZoneListModel>();
}

bool ZonesController::addOrUpdateZone(Zone &zone)
{
    if (!confZoneManager()->addOrUpdateZone(zone)) {
        showErrorMessage(tr("Cannot edit Zone"));
        return false;
    }
    return true;
}

void ZonesController::deleteZone(int zoneId)
{
    if (!confZoneManager()->deleteZone(zoneId)) {
        showErrorMessage(tr("Cannot delete Zone"));
    }
}

bool ZonesController::updateZoneName(int zoneId, const QString &zoneName)
{
    if (!confZoneManager()->updateZoneName(zoneId, zoneName)) {
        showErrorMessage(tr("Cannot update Zone's name"));
        return false;
    }
    return true;
}
