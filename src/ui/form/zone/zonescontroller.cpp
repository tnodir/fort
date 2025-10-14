#include "zonescontroller.h"

#include <conf/confzonemanager.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

namespace {

void showErrorMessage(const QString &errorMessage)
{
    windowManager()->showErrorBox(errorMessage, ZonesController::tr("Zone Configuration Error"));
}

}

ZonesController::ZonesController(QObject *parent) : BaseController(parent) { }

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
