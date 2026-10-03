#include "speedlimitscontroller.h"

#include <conf/confspeedlimitmanager.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/speedlimitlistmodel.h>

using namespace Fort;

namespace {

void showErrorMessage(const QString &errorMessage)
{
    windowManager()->showErrorBox(
            errorMessage, SpeedLimitsController::tr("Speed Limit Configuration Error"));
}

}

SpeedLimitsController::SpeedLimitsController(QObject *parent) : BaseController(parent) { }

bool SpeedLimitsController::addOrUpdateSpeedLimit(SpeedLimit &limit)
{
    if (!confSpeedLimitManager()->addOrUpdateSpeedLimit(limit)) {
        showErrorMessage(tr("Cannot edit Speed Limit"));
        return false;
    }
    return true;
}

void SpeedLimitsController::deleteSpeedLimit(int limitId)
{
    if (!confSpeedLimitManager()->deleteSpeedLimit(limitId)) {
        showErrorMessage(tr("Cannot delete Speed Limit"));
    }
}

bool SpeedLimitsController::updateSpeedLimitName(int limitId, const QString &name)
{
    if (!confSpeedLimitManager()->updateSpeedLimitName(limitId, name)) {
        showErrorMessage(tr("Cannot update Speed Limit's name"));
        return false;
    }
    return true;
}

void SpeedLimitsController::readSpeedLimitStatus()
{
    QByteArray buf;

    if (!driverManager()->readSpeedLimitStatus(buf)) {
        buf.clear();
    }

    speedLimitListModel()->setStatusData(buf);
}

void SpeedLimitsController::clearSpeedLimitStatus()
{
    speedLimitListModel()->setStatusData({});
}
